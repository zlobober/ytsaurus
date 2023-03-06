from yt_env_setup import (
    YTEnvSetup,
    Restarter,
    CONTROLLER_AGENTS_SERVICE,
)
from yt_commands import (
    authors, wait, create,
    ls, get,
    set, exists, write_table,
    map, reduce)


##################################################################


class TestSchedulerConfig(YTEnvSetup):
    NUM_MASTERS = 1
    NUM_NODES = 5
    NUM_SCHEDULERS = 1

    DELTA_SCHEDULER_CONFIG = {
        "scheduler": {
            "event_log": {"retry_backoff_time": 7, "flush_period": 5000},
        },
        "addresses": [("ipv4", "127.0.0.1"), ("ipv6", "::1")],
    }

    DELTA_CONTROLLER_AGENT_CONFIG = {
        "controller_agent": {
            "event_log": {"retry_backoff_time": 7, "flush_period": 5000},
            "operation_options": {"spec_template": {"data_weight_per_job": 1000}},
            "map_operation_options": {
                "spec_template": {
                    "data_weight_per_job": 2000,
                    "max_failed_job_count": 10,
                }
            },
            "environment": {"TEST_VAR": "10"},
        },
        "addresses": [("ipv4", "127.0.0.1"), ("ipv6", "::1")],
    }

    @authors("ignat")
    def test_basic(self):
        orchid_scheduler_config = "//sys/scheduler/orchid/scheduler/config"
        assert get("{0}/event_log/flush_period".format(orchid_scheduler_config)) == 5000
        assert get("{0}/event_log/retry_backoff_time".format(orchid_scheduler_config)) == 7

        set("//sys/scheduler/config", {"event_log": {"flush_period": 10000}})

        wait(lambda: get("{0}/event_log/flush_period".format(orchid_scheduler_config)) == 10000)
        wait(lambda: get("{0}/event_log/retry_backoff_time".format(orchid_scheduler_config)) == 7)

        set("//sys/scheduler/config", {})

        wait(lambda: get("{0}/event_log/flush_period".format(orchid_scheduler_config)) == 5000)
        wait(lambda: get("{0}/event_log/retry_backoff_time".format(orchid_scheduler_config)) == 7)

    @authors("ignat")
    def test_addresses(self):
        addresses = get("//sys/scheduler/@addresses")
        assert addresses["ipv4"].startswith("127.0.0.1:")
        assert addresses["ipv6"].startswith("::1:")

    @authors("ignat")
    def test_specs(self):
        create("table", "//tmp/t_in")
        write_table("<append=true;sorted_by=[foo]>//tmp/t_in", {"foo": "bar"})

        create("table", "//tmp/t_out")

        op = map(command="sleep 1000", in_=["//tmp/t_in"], out="//tmp/t_out", track=False, fail_fast=False)

        full_spec_path = "//sys/scheduler/orchid/scheduler/operations/{0}/full_spec".format(op.id)
        wait(lambda: exists(full_spec_path))

        assert get("{}/data_weight_per_job".format(full_spec_path)) == 2000
        assert get("{}/max_failed_job_count".format(full_spec_path)) == 10

        op.abort()

        op = reduce(
            command="sleep 1000",
            in_=["//tmp/t_in"],
            out="//tmp/t_out",
            reduce_by=["foo"],
            track=False,
            fail_fast=False,
        )
        wait(lambda: op.get_state() == "running")

        full_spec_path = "//sys/scheduler/orchid/scheduler/operations/{0}/full_spec".format(op.id)
        wait(lambda: exists(full_spec_path))

        assert get("{}/data_weight_per_job".format(full_spec_path)) == 1000
        assert get("{}/max_failed_job_count".format(full_spec_path)) == 10

        with Restarter(self.Env, CONTROLLER_AGENTS_SERVICE):
            pass

        op.ensure_running()

        assert get("{}/data_weight_per_job".format(full_spec_path)) == 1000
        assert get("{}/max_failed_job_count".format(full_spec_path)) == 10

        op.abort()

    @authors("ignat")
    def test_unrecognized_spec(self):
        create("table", "//tmp/t_in")
        write_table("//tmp/t_in", [{"a": "b"}])
        create("table", "//tmp/t_out")
        op = map(
            command="sleep 1000",
            in_=["//tmp/t_in"],
            out="//tmp/t_out",
            track=False,
            spec={"xxx": "yyy"},
        )

        wait(lambda: exists(op.get_path() + "/@unrecognized_spec"))
        assert get(op.get_path() + "/@unrecognized_spec") == {"xxx": "yyy"}

    @authors("ignat")
    def test_brief_progress(self):
        create("table", "//tmp/t_in")
        write_table("//tmp/t_in", [{"a": "b"}])
        create("table", "//tmp/t_out")
        op = map(command="sleep 1000", in_=["//tmp/t_in"], out="//tmp/t_out", track=False)

        wait(lambda: exists(op.get_path() + "/@brief_progress"))
        assert "jobs" in list(get(op.get_path() + "/@brief_progress"))

    @authors("ignat")
    def test_cypress_config(self):
        create("table", "//tmp/t_in")
        write_table("<append=true>//tmp/t_in", {"foo": "bar"})
        create("table", "//tmp/t_out")

        op = map(command="cat", in_=["//tmp/t_in"], out="//tmp/t_out", fail_fast=False)
        assert get(op.get_path() + "/@full_spec/data_weight_per_job") == 2000
        assert get(op.get_path() + "/@full_spec/max_failed_job_count") == 10

        set(
            "//sys/controller_agents/config",
            {
                "map_operation_options": {"spec_template": {"max_failed_job_count": 50}},
                "environment": {"OTHER_VAR": "20"},
            },
        )

        instances = ls("//sys/controller_agents/instances")
        for instance in instances:
            config_path = "//sys/controller_agents/instances/{0}/orchid/controller_agent/config".format(instance)
            wait(
                lambda: exists(config_path + "/environment/OTHER_VAR")
                and get(config_path + "/environment/OTHER_VAR") == "20"
            )

            environment = get(config_path + "/environment")
            assert environment["TEST_VAR"] == "10"
            assert environment["OTHER_VAR"] == "20"

            assert get(config_path + "/map_operation_options/spec_template/max_failed_job_count") == 50

        op = map(command="cat", in_=["//tmp/t_in"], out="//tmp/t_out", fail_fast=False)
        assert get(op.get_path() + "/@full_spec/data_weight_per_job") == 2000
        assert get(op.get_path() + "/@full_spec/max_failed_job_count") == 50

    @authors("ignat")
    def test_min_spare_job_resources_on_node(self):
        orchid_scheduler_config = "//sys/scheduler/orchid/scheduler/config"
        min_spare_job_resources = get("{0}/min_spare_job_resources_on_node".format(orchid_scheduler_config))
        assert min_spare_job_resources["cpu"] == 1.0
        assert min_spare_job_resources["user_slots"] == 1
        assert min_spare_job_resources["memory"] == 256 * 1024 * 1024

        set("//sys/scheduler/config/min_spare_job_resources_on_node", {"user_slots": 2})
        wait(lambda: get("{0}/min_spare_job_resources_on_node".format(orchid_scheduler_config)) == {"user_slots": 2})


class TestSchedulerConfigWithoutAllocationService(TestSchedulerConfig):
    DELTA_NODE_CONFIG = {
        "exec_agent": {
            "scheduler_connector": {
                "use_allocation_tracker_service": False,
            },
        },
    }