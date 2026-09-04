########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(cppzmq_FIND_QUIETLY)
    set(cppzmq_MESSAGE_MODE VERBOSE)
else()
    set(cppzmq_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cppzmqTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${cppzmq_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(cppzmq_VERSION_STRING "4.11.0")
set(cppzmq_INCLUDE_DIRS ${cppzmq_INCLUDE_DIRS_RELEASE} )
set(cppzmq_INCLUDE_DIR ${cppzmq_INCLUDE_DIRS_RELEASE} )
set(cppzmq_LIBRARIES ${cppzmq_LIBRARIES_RELEASE} )
set(cppzmq_DEFINITIONS ${cppzmq_DEFINITIONS_RELEASE} )


# Definition of extra CMake variables from cmake_extra_variables


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${cppzmq_BUILD_MODULES_PATHS_RELEASE} )
    message(${cppzmq_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


