cmake_minimum_required(VERSION 3.15)
project(myapp CXX)

# Levity LGPL dynamic-link integration fixture (examples/conan-lgpl-integration):
# libiconv/1.18 (LGPL-2.1-or-later) is *always* a Conan requirement (see
# conanfile.py) so it always appears in conan.lock and in the Trivy-generated
# SBOM. This option only controls whether the *application* actually links
# and uses it, which is the condition IsDynamicallyLinked measures.
option(ENABLE_LGPL_LINK "Link the application against the LGPL libiconv dependency" ON)

# Find required packages
find_package(spdlog CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

# Create compression library
add_library(compression
    src/compression.cpp
    src/compression.h
)

target_link_libraries(compression PUBLIC
    ZLIB::ZLIB
    spdlog::spdlog
)

target_include_directories(compression PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Main executable
add_executable(myapp src/main.cpp)

target_link_libraries(myapp PRIVATE
    compression
    spdlog::spdlog
    ZLIB::ZLIB
)

if(ENABLE_LGPL_LINK)
    # Conan's libiconv/1.18 recipe exports the CMake target as Iconv::Iconv
    # (mirroring the standard CMake FindIconv module name), not
    # libiconv::libiconv -- confirmed from the actual CMakeDeps generate()
    # output, not assumed.
    find_package(Iconv REQUIRED)
    target_sources(myapp PRIVATE src/lgpl_link.cpp)
    target_link_libraries(myapp PRIVATE Iconv::Iconv)
    target_compile_definitions(myapp PRIVATE LEVITY_ENABLE_LGPL_LINK=1)
else()
    target_compile_definitions(myapp PRIVATE LEVITY_ENABLE_LGPL_LINK=0)
endif()


include(CTest)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
