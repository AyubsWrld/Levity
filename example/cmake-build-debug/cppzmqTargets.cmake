# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/cppzmq-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${cppzmq_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${cppzmq_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET cppzmq)
    add_library(cppzmq INTERFACE IMPORTED)
    message(${cppzmq_MESSAGE_MODE} "Conan: Target declared 'cppzmq'")
endif()
if(NOT TARGET cppzmq-static)
    add_library(cppzmq-static INTERFACE IMPORTED)
    set_property(TARGET cppzmq-static PROPERTY INTERFACE_LINK_LIBRARIES cppzmq)
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/cppzmq-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()