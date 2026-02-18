load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "stats",
    srcs = ["stats.cpp"],
    hdrs = ["include/stats.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/logger:logger",
        "//modules/capture:capture",
    ],
    copts = [
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-fPIC",
    ],
    linkopts = [
        "-shared",
    ],
)
