load("@rules_cc//cc:defs.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "connect4AI",
    srcs = ["connect4AI.cpp"],
    hdrs = ["connect4AI.h"],
    deps = [
        "//proto:game_cc_proto",
    ],
)

cc_binary(
    name = "connect4Client",
    srcs = ["connect4Client.cpp"],
    deps = [
        ":connect4AI",
        "@com_github_grpc_grpc//:grpc++",
        "//proto:game_cc_proto",
        "//proto:game_cc_grpc",
    ],
)

cc_test(
    name = "connect4Test",
    srcs = ["connect4Test.cpp"],
    deps = [
        ":connect4AI",
        "@gtest//:gtest",
        "@gtest//:gtest_main"
    ],
)
