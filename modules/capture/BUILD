load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "capture",
    srcs = ["capture.cpp"],
    hdrs = ["include/capture.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/logger:logger",
        "@backward//:backward",
    ],
    copts = [
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-fPIC",
    ],
    linkopts = [
        "-shared",
        "-Wl,--no-as-needed",  // 确保 dlsym 正常工作
    ],
)
