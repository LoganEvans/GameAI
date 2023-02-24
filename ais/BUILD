package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fourInARowAI",
    srcs = ["fourInARow.cpp"],
    hdrs = ["fourInARow.h"],
)

cc_binary(
    name = "fourInARowClient",
    srcs = ["fourInARowClient.cpp"],
    deps = [
        ":fourInARowAI",
        "@com_github_grpc_grpc//:grpc++",
        "//proto:game_cc_proto",
        "//proto:game_cc_grpc",
    ],
)
