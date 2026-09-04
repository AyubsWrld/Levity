# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(zeromq_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(zeromq_FRAMEWORKS_FOUND_RELEASE "${zeromq_FRAMEWORKS_RELEASE}" "${zeromq_FRAMEWORK_DIRS_RELEASE}")

set(zeromq_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET zeromq_DEPS_TARGET)
    add_library(zeromq_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET zeromq_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${zeromq_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${zeromq_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### zeromq_DEPS_TARGET to all of them
conan_package_library_targets("${zeromq_LIBS_RELEASE}"    # libraries
                              "${zeromq_LIB_DIRS_RELEASE}" # package_libdir
                              "${zeromq_BIN_DIRS_RELEASE}" # package_bindir
                              "${zeromq_LIBRARY_TYPE_RELEASE}"
                              "${zeromq_IS_HOST_WINDOWS_RELEASE}"
                              zeromq_DEPS_TARGET
                              zeromq_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "zeromq"    # package_name
                              "${zeromq_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${zeromq_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT libzmq-static #############

        set(zeromq_libzmq-static_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(zeromq_libzmq-static_FRAMEWORKS_FOUND_RELEASE "${zeromq_libzmq-static_FRAMEWORKS_RELEASE}" "${zeromq_libzmq-static_FRAMEWORK_DIRS_RELEASE}")

        set(zeromq_libzmq-static_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET zeromq_libzmq-static_DEPS_TARGET)
            add_library(zeromq_libzmq-static_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET zeromq_libzmq-static_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_DEPENDENCIES_RELEASE}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'zeromq_libzmq-static_DEPS_TARGET' to all of them
        conan_package_library_targets("${zeromq_libzmq-static_LIBS_RELEASE}"
                              "${zeromq_libzmq-static_LIB_DIRS_RELEASE}"
                              "${zeromq_libzmq-static_BIN_DIRS_RELEASE}" # package_bindir
                              "${zeromq_libzmq-static_LIBRARY_TYPE_RELEASE}"
                              "${zeromq_libzmq-static_IS_HOST_WINDOWS_RELEASE}"
                              zeromq_libzmq-static_DEPS_TARGET
                              zeromq_libzmq-static_LIBRARIES_TARGETS
                              "_RELEASE"
                              "zeromq_libzmq-static"
                              "${zeromq_libzmq-static_NO_SONAME_MODE_RELEASE}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET libzmq-static
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_LIBRARIES_TARGETS}>
                     )

        if("${zeromq_libzmq-static_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET libzmq-static
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         zeromq_libzmq-static_DEPS_TARGET)
        endif()

        set_property(TARGET libzmq-static APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_LINKER_FLAGS_RELEASE}>)
        set_property(TARGET libzmq-static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_INCLUDE_DIRS_RELEASE}>)
        set_property(TARGET libzmq-static APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_LIB_DIRS_RELEASE}>)
        set_property(TARGET libzmq-static APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_COMPILE_DEFINITIONS_RELEASE}>)
        set_property(TARGET libzmq-static APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${zeromq_libzmq-static_COMPILE_OPTIONS_RELEASE}>)


    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET libzmq-static APPEND PROPERTY INTERFACE_LINK_LIBRARIES libzmq-static)

########## For the modules (FindXXX)
set(zeromq_LIBRARIES_RELEASE libzmq-static)
