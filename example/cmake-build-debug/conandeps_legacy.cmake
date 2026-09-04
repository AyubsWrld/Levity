message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(cppzmq)
find_package(protobuf)
find_package(httplib)
find_package(nlohmann_json)
find_package(GTest)

set(CONANDEPS_LEGACY  cppzmq  protobuf::protobuf  httplib::httplib  nlohmann_json::nlohmann_json  gtest::gtest )