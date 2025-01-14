//
// Copyright 2023 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "src/cpp/server/backend_metric_recorder.h"

#include <inttypes.h>

#include <functional>
#include <memory>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <type_traits>
#include <utility>

#include <grpc/support/log.h>
#include <grpcpp/ext/call_metric_recorder.h>
#include <grpcpp/ext/server_metric_recorder.h>

#include "src/core/ext/filters/client_channel/lb_policy/backend_metric_data.h"
#include "src/core/lib/debug/trace.h"

using grpc_core::BackendMetricData;

namespace {
// All utilization values must be in [0, 1].
bool IsUtilizationValid(double utilization) {
  return utilization >= 0.0 && utilization <= 1.0;
}

// QPS must be in [0, infy).
bool IsQpsValid(double qps) { return qps >= 0.0; }

grpc_core::TraceFlag grpc_backend_metric_trace(false, "backend_metric");
}  // namespace

namespace grpc {
namespace experimental {

std::unique_ptr<ServerMetricRecorder> ServerMetricRecorder::Create() {
  return std::unique_ptr<ServerMetricRecorder>(new ServerMetricRecorder());
}

ServerMetricRecorder::ServerMetricRecorder()
    : metric_state_(std::make_shared<const BackendMetricDataState>()) {}

void ServerMetricRecorder::UpdateBackendMetricDataState(
    std::function<void(BackendMetricData*)> updater) {
  internal::MutexLock lock(&mu_);
  auto new_state = std::make_shared<BackendMetricDataState>(*metric_state_);
  updater(&new_state->data);
  ++new_state->sequence_number;
  metric_state_ = std::move(new_state);
}

void ServerMetricRecorder::SetCpuUtilization(double value) {
  if (!IsUtilizationValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] CPU utilization rejected: %f", this, value);
    }
    return;
  }
  UpdateBackendMetricDataState(
      [value](BackendMetricData* data) { data->cpu_utilization = value; });
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] CPU utilization set: %f", this, value);
  }
}

void ServerMetricRecorder::SetMemoryUtilization(double value) {
  if (!IsUtilizationValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] Mem utilization rejected: %f", this, value);
    }
    return;
  }
  UpdateBackendMetricDataState(
      [value](BackendMetricData* data) { data->mem_utilization = value; });
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Mem utilization set: %f", this, value);
  }
}

void ServerMetricRecorder::SetQps(double value) {
  if (!IsQpsValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] QPS rejected: %f", this, value);
    }
    return;
  }
  UpdateBackendMetricDataState(
      [value](BackendMetricData* data) { data->qps = value; });
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] QPS set: %f", this, value);
  }
}

void ServerMetricRecorder::SetNamedUtilization(string_ref name, double value) {
  if (!IsUtilizationValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] Named utilization rejected: %f name: %s", this,
              value, TString(name.data(), name.size()).c_str());
    }
    return;
  }
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Named utilization set: %f name: %s", this, value,
            TString(name.data(), name.size()).c_str());
  }
  UpdateBackendMetricDataState([name, value](BackendMetricData* data) {
    data->utilization[y_absl::string_view(name.data(), name.size())] = value;
  });
}

void ServerMetricRecorder::SetAllNamedUtilization(
    std::map<string_ref, double> named_utilization) {
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] All named utilization updated. size: %" PRIuPTR,
            this, named_utilization.size());
  }
  UpdateBackendMetricDataState(
      [utilization = std::move(named_utilization)](BackendMetricData* data) {
        data->utilization.clear();
        for (const auto& u : utilization) {
          data->utilization[y_absl::string_view(u.first.data(), u.first.size())] =
              u.second;
        }
      });
}

void ServerMetricRecorder::ClearCpuUtilization() {
  UpdateBackendMetricDataState(
      [](BackendMetricData* data) { data->cpu_utilization = -1; });
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] CPU utilization cleared.", this);
  }
}

void ServerMetricRecorder::ClearMemoryUtilization() {
  UpdateBackendMetricDataState(
      [](BackendMetricData* data) { data->mem_utilization = -1; });
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Mem utilization cleared.", this);
  }
}

void ServerMetricRecorder::ClearQps() {
  UpdateBackendMetricDataState([](BackendMetricData* data) { data->qps = -1; });
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] QPS utilization cleared.", this);
  }
}

void ServerMetricRecorder::ClearNamedUtilization(string_ref name) {
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Named utilization cleared. name: %s", this,
            TString(name.data(), name.size()).c_str());
  }
  UpdateBackendMetricDataState([name](BackendMetricData* data) {
    data->utilization.erase(y_absl::string_view(name.data(), name.size()));
  });
}

grpc_core::BackendMetricData ServerMetricRecorder::GetMetrics() const {
  auto result = GetMetricsIfChanged();
  return result->data;
}

std::shared_ptr<const ServerMetricRecorder::BackendMetricDataState>
ServerMetricRecorder::GetMetricsIfChanged() const {
  std::shared_ptr<const BackendMetricDataState> result;
  {
    internal::MutexLock lock(&mu_);
    result = metric_state_;
  }
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    const auto& data = result->data;
    gpr_log(GPR_INFO,
            "[%p] GetMetrics() returned: seq:%" PRIu64
            " cpu:%f mem:%f qps:%f utilization size: %" PRIuPTR,
            this, result->sequence_number, data.cpu_utilization,
            data.mem_utilization, data.qps, data.utilization.size());
  }
  return result;
}

}  // namespace experimental

experimental::CallMetricRecorder&
BackendMetricState::RecordCpuUtilizationMetric(double value) {
  if (!IsUtilizationValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] CPU utilization value rejected: %f", this, value);
    }
    return *this;
  }
  cpu_utilization_.store(value, std::memory_order_relaxed);
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] CPU utilization recorded: %f", this, value);
  }
  return *this;
}

experimental::CallMetricRecorder&
BackendMetricState::RecordMemoryUtilizationMetric(double value) {
  if (!IsUtilizationValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] Mem utilization value rejected: %f", this, value);
    }
    return *this;
  }
  mem_utilization_.store(value, std::memory_order_relaxed);
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Mem utilization recorded: %f", this, value);
  }
  return *this;
}

experimental::CallMetricRecorder& BackendMetricState::RecordQpsMetric(
    double value) {
  if (!IsQpsValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] QPS value rejected: %f", this, value);
    }
    return *this;
  }
  qps_.store(value, std::memory_order_relaxed);
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] QPS recorded: %f", this, value);
  }
  return *this;
}

experimental::CallMetricRecorder& BackendMetricState::RecordUtilizationMetric(
    string_ref name, double value) {
  if (!IsUtilizationValid(value)) {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
      gpr_log(GPR_INFO, "[%p] Utilization value rejected: %s %f", this,
              TString(name.data(), name.length()).c_str(), value);
    }
    return *this;
  }
  internal::MutexLock lock(&mu_);
  y_absl::string_view name_sv(name.data(), name.length());
  utilization_[name_sv] = value;
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Utilization recorded: %s %f", this,
            TString(name_sv).c_str(), value);
  }
  return *this;
}

experimental::CallMetricRecorder& BackendMetricState::RecordRequestCostMetric(
    string_ref name, double value) {
  internal::MutexLock lock(&mu_);
  y_absl::string_view name_sv(name.data(), name.length());
  request_cost_[name_sv] = value;
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO, "[%p] Request cost recorded: %s %f", this,
            TString(name_sv).c_str(), value);
  }
  return *this;
}

BackendMetricData BackendMetricState::GetBackendMetricData() {
  // Merge metrics from the ServerMetricRecorder first since metrics recorded
  // to CallMetricRecorder takes a higher precedence.
  BackendMetricData data;
  if (server_metric_recorder_ != nullptr) {
    data = server_metric_recorder_->GetMetrics();
  }
  // Only overwrite if the value is set i.e. in the valid range.
  const double cpu = cpu_utilization_.load(std::memory_order_relaxed);
  if (IsUtilizationValid(cpu)) {
    data.cpu_utilization = cpu;
  }
  const double mem = mem_utilization_.load(std::memory_order_relaxed);
  if (IsUtilizationValid(mem)) {
    data.mem_utilization = mem;
  }
  const double qps = qps_.load(std::memory_order_relaxed);
  if (IsQpsValid(qps)) {
    data.qps = qps;
  }
  {
    internal::MutexLock lock(&mu_);
    data.utilization = std::move(utilization_);
    data.request_cost = std::move(request_cost_);
  }
  if (GRPC_TRACE_FLAG_ENABLED(grpc_backend_metric_trace)) {
    gpr_log(GPR_INFO,
            "[%p] Backend metric data returned: cpu:%f mem:%f qps:%f "
            "utilization size:%" PRIuPTR " request_cost size:%" PRIuPTR,
            this, data.cpu_utilization, data.mem_utilization, data.qps,
            data.utilization.size(), data.request_cost.size());
  }
  return data;
}

}  // namespace grpc
