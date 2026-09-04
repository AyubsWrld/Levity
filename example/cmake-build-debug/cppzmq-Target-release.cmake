# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(cppzmq_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(cppzmq_FRAMEWORKS_FOUND_RELEASE "${cppzmq_FRAMEWORKS_RELEASE}" "${cppzmq_FRAMEWORK_DIRS_RELEASE}")

set(cppzmq_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET cppzmq_DEPS_TARGET)
    add_library(cppzmq_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET cppzmq_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${cppzmq_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${cppzmq_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:libzmq-static>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### cppzmq_DEPS_TARGET to all of them
conan_package_library_targets("${cppzmq_LIBS_RELEASE}"    # libraries
                              "${cppzmq_LIB_DIRS_RELEASE}" # package_libdir
                              "${cppzmq_BIN_DIRS_RELEASE}" # package_bindir
                              "${cppzmq_LIBRARY_TYPE_RELEASE}"
                              "${cppzmq_IS_HOST_WINDOWS_RELEASE}"
                              cppzmq_DEPS_TARGET
                              cppzmq_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "cppzmq"    # package_name
                              "${cppzmq_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${cppzmq_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET cppzmq
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${cppzmq_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${cppzmq_LIBRARIES_TARGETS}>
                 )

    if("${cppzmq_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET cppzmq
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     cppzmq_DEPS_TARGET)
    endif()

    set_property(TARGET cppzmq
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${cppzmq_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET cppzmq
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${cppzmq_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET cppzmq
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${cppzmq_LIB_DIRS_RELEASE}>)
    set_property(TARGET cppzmq
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${cppzmq_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET cppzmq
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${cppzmq_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(cppzmq_LIBRARIES_RELEASE cppzmq)
