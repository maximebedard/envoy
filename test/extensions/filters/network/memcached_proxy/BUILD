licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_mock",
    "envoy_package",
)
load(
    "//test/extensions:extensions_build_system.bzl",
    "envoy_extension_cc_test",
    "envoy_extension_cc_test_binary",
)

envoy_package()

envoy_extension_cc_test(
    name = "codec_impl_test",
    srcs = ["codec_impl_test.cc"],
    extension_name = "envoy.filters.network.memcached_proxy",
    deps = [
        "//source/common/buffer:buffer_lib",
        "//source/common/json:json_loader_lib",
        "//source/extensions/filters/network/memcached_proxy:codec_lib",
    ],
)

envoy_cc_mock(
    name = "memcached_mocks",
    srcs = ["mocks.cc"],
    hdrs = ["mocks.h"],
    deps = [
        "//source/common/common:assert_lib",
        "//source/extensions/filters/network/memcached_proxy:codec_lib",
        "//source/extensions/filters/network/memcached_proxy:conn_pool_interface",
    ],
)


envoy_extension_cc_test(
    name = "proxy_test",
    srcs = ["proxy_test.cc"],
    extension_name = "envoy.filters.network.memcached_proxy",
    deps = [
        ":memcached_mocks",
        "//source/common/config:filter_json_lib",
        "//source/common/event:dispatcher_lib",
        "//source/extensions/filters/network/memcached_proxy:proxy_lib",
        "//test/mocks:common_lib",
        "//test/mocks/network:network_mocks",
        "//test/mocks/upstream:upstream_mocks",
        "//test/test_common:utility_lib",
    ],
)

envoy_extension_cc_test(
    name = "conn_pool_impl_test",
    srcs = ["conn_pool_impl_test.cc"],
    extension_name = "envoy.filters.network.memcached_proxy",
    deps = [
        ":memcached_mocks",
        "//source/common/event:dispatcher_lib",
        "//source/common/network:utility_lib",
        "//source/common/upstream:upstream_includes",
        "//source/common/upstream:upstream_lib",
        "//source/extensions/filters/network/memcached_proxy:conn_pool_lib",
        "//test/mocks/network:network_mocks",
        "//test/mocks/thread_local:thread_local_mocks",
        "//test/mocks/upstream:upstream_mocks",
    ],
)

