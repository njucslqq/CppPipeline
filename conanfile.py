from conan import ConanFile


class MemoryTracerRecipe(ConanFile):
    name = "memory_tracer"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    requires = "spdlog/1.12.0", "nlohmann_json/3.11.2", "fmt/10.1.1", "backward-cpp/1.6"
    generators = "bazel", "json"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": True, "fPIC": True}

    def configure(self):
        if self.options.shared:
            self.options["spdlog"].shared = True
            self.options["fmt"].shared = True

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="lib", src="lib")
        self.copy("*.so*", dst="lib", src="lib")
