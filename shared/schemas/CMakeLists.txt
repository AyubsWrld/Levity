add_library(edge_schemas STATIC
    telemetry.proto
    commands.proto
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Make protoc come from the same Conan protobuf package as libprotobuf. This avoids accidentally
# generating code with a different system-installed protoc version.
if(TARGET protobuf::protoc)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _edge_build_type_upper)
    if(DEFINED protobuf_PACKAGE_FOLDER_${_edge_build_type_upper})
        set(_edge_protoc "${protobuf_PACKAGE_FOLDER_${_edge_build_type_upper}}/bin/protoc${CMAKE_EXECUTABLE_SUFFIX}")
        if(EXISTS "${_edge_protoc}")
            set_property(TARGET protobuf::protoc PROPERTY IMPORTED_LOCATION "${_edge_protoc}")
        endif()
    endif()
    unset(_edge_build_type_upper)
    unset(_edge_protoc)
endif()

protobuf_generate(
    TARGET edge_schemas
    LANGUAGE cpp
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(edge_schemas PUBLIC protobuf::libprotobuf)
target_include_directories(edge_schemas SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

# edge_schemas compiles protoc-generated .pb.cc sources we do not own and cannot annotate; the
# repository's naming/style checks do not apply to third-party-generated code, so clang-tidy is
# disabled for this target only (the generated headers are still exempt from lint diagnostics via
# the .clang-tidy HeaderFilterRegex when included from other targets).
set_target_properties(edge_schemas PROPERTIES CXX_CLANG_TIDY "")

