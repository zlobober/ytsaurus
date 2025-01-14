#pragma once

#include <yt/yt/ytlib/job_prober_client/public.h>

#include <yt/yt/core/actions/public.h>
#include <yt/yt/core/concurrency/public.h>
#include <yt/yt/core/rpc/public.h>

namespace NYT::NJobProber {

////////////////////////////////////////////////////////////////////////////////

NRpc::IServicePtr CreateJobProberService(
    NJobProberClient::IJobProbePtr jobProbe,
    IInvokerPtr controlInvoker);

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NJobProber
