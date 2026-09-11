from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd


class MyAppRecipe(ConanFile):
    name = "myapp"
    version = "1.0"
    package_type = "application"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"

    # libiconv is a real LGPL-2.1-or-later dependency of this integration
    # fixture (see Levity's examples/conan-lgpl-integration/README.md). It
    # must be built shared so its SONAME is observable in DT_NEEDED -- pinned
    # here rather than relying on whatever the recipe's default happens to be.
    default_options = {"libiconv/*:shared": True}

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("spdlog/[>1.14]")
        self.requires("zlib/[>=1.2.11]")
        self.requires("libiconv/1.18")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3]")
        self.test_requires("gtest/[>=1.17]")

    def validate(self):
        check_min_cppstd(self, 17)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.ctest()

    def package(self):
        cmake = CMake(self)
        cmake.install()
