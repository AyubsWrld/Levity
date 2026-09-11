"""Conan 2 recipe for the Tier 1 Slew-to-Cue MVP.

Third-party C++ dependencies are managed exclusively through Conan 2 with exact, reviewed versions.
Consumers use the ``CMakeDeps``/``CMakeToolchain`` generators and ``find_package()`` imported targets.

Canonical developer workflow (see README.md):

    conan install . --output-folder=cmake-build-debug --build=missing \\
        -s build_type=Debug -s compiler.cppstd=23
"""
from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class ConanProject(ConanFile):
    name = "edge-isr-platform"
    version = "0.1.0"
    description = "Modular Edge ISR API Platform - Tier 1 Slew-to-Cue MVP"
    license = "Proprietary"
    settings = "os", "compiler", "build_type", "arch"

    # Exact pinned versions. Never use version ranges or 'latest' resolution.
    requires = (
        "cppzmq/4.11.0",
        "fmt/10.2.1",
        "protobuf/6.30.1",
        "cpp-httplib/0.30.0",
        "nlohmann_json/3.11.3",
        "gtest/1.18.0",
    )

    # CurveZMQ is not used: all Tier 1 IPC is local Unix-domain sockets inside one trust boundary.
    default_options = {
        "zeromq/*:shared": True,  # Force dynamic linking for libzmq
        "zeromq/*:encryption": False,
        "zeromq/*:with_websocket": False,
        "zeromq/*:with_draft_api": False,
        "cpp-httplib/*:with_openssl": False,
        "cpp-httplib/*:with_zlib": False,
        "cpp-httplib/*:with_brotli": False,
        "gtest/*:build_gmock": True,
    }

    def layout(self):
        self.folders.generators = "."

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.generate()
