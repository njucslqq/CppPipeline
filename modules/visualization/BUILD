load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "visualization",
    srcs = ["visualization.cpp"],
    hdrs = ["include/visualization.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/logger:logger",
        "//modules/stats:stats",
        "//modules/storage:storage",
    ],
    copts = [
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-fPIC",
    ],
    linkopts = [
        "-shared",
        "-lpthread",
    ],
)
