load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "storage",
    srcs = ["storage.cpp"],
    hdrs = ["include/storage.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//modules/logger:logger",
        "//modules/capture:capture",
        "@nlohmann_json//:nlohmann_json",
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
