load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "logger",
    srcs = ["logger.cpp"],
    hdrs = ["include/logger.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "@spdlog//:spdlog",
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
