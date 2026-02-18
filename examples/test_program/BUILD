load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "test_program",
    srcs = ["main.cpp"],
    deps = [
        "//modules/logger:logger",
        "//modules/capture:capture",
        "//modules/storage:storage",
        "//modules/stats:stats",
        "//modules/visualization:visualization",
    ],
    copts = [
        "-std=c++17",
        "-Wall",
        "-Wextra",
    ],
    linkopts = [
        "-ldl",  # for dlsym
        "-lpthread",
    ],
    data = [
        "//modules/logger:logger",
        "//modules/capture:capture",
        "//modules/storage:storage",
        "//modules/stats:stats",
        "//modules/visualization:visualization",
    ],
)
