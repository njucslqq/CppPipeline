workspace(name = "memory_tracer")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Conan integration
http_archive(
    name = "rules_conan",
    sha256 = "c265071c8f8806445834943f0c6d1df2bea03a958895494269d447d339633086",
    urls = ["https://github.com/uguisu/rules_conan/releases/download/0.9.1/rules_conan-0.9.1.tar.gz"],
    strip_prefix = "rules_conan-0.9.1",
)

load("@rules_conan//conan:defs.bzl", "conan_install")

conan_install(
    name = "conan_deps",
    generates = "conan_dependencies.bzl",
    generator = "bazel",
)

load("@conan_deps//:conan_dependencies.bzl", "load_conan_dependencies")
load_conan_dependencies()

# Google Test for testing
http_archive(
    name = "com_google_googletest",
    urls = ["https://github.com/google/googletest/archive/release-1.12.1.zip"],
    strip_prefix = "googletest-release-1.12.1",
)
