########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(cppzmq_COMPONENT_NAMES "")
if(DEFINED cppzmq_FIND_DEPENDENCY_NAMES)
  list(APPEND cppzmq_FIND_DEPENDENCY_NAMES ZeroMQ)
  list(REMOVE_DUPLICATES cppzmq_FIND_DEPENDENCY_NAMES)
else()
  set(cppzmq_FIND_DEPENDENCY_NAMES ZeroMQ)
endif()
set(ZeroMQ_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(cppzmq_PACKAGE_FOLDER_RELEASE "/root/.conan2/p/cppzm72f3364cb0c0b/p")
set(cppzmq_BUILD_MODULES_PATHS_RELEASE )


set(cppzmq_INCLUDE_DIRS_RELEASE "${cppzmq_PACKAGE_FOLDER_RELEASE}/include")
set(cppzmq_RES_DIRS_RELEASE )
set(cppzmq_DEFINITIONS_RELEASE )
set(cppzmq_SHARED_LINK_FLAGS_RELEASE )
set(cppzmq_EXE_LINK_FLAGS_RELEASE )
set(cppzmq_OBJECTS_RELEASE )
set(cppzmq_COMPILE_DEFINITIONS_RELEASE )
set(cppzmq_COMPILE_OPTIONS_C_RELEASE )
set(cppzmq_COMPILE_OPTIONS_CXX_RELEASE )
set(cppzmq_LIB_DIRS_RELEASE )
set(cppzmq_BIN_DIRS_RELEASE )
set(cppzmq_LIBRARY_TYPE_RELEASE UNKNOWN)
set(cppzmq_IS_HOST_WINDOWS_RELEASE 0)
set(cppzmq_LIBS_RELEASE )
set(cppzmq_SYSTEM_LIBS_RELEASE )
set(cppzmq_FRAMEWORK_DIRS_RELEASE )
set(cppzmq_FRAMEWORKS_RELEASE )
set(cppzmq_BUILD_DIRS_RELEASE )
set(cppzmq_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(cppzmq_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${cppzmq_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${cppzmq_COMPILE_OPTIONS_C_RELEASE}>")
set(cppzmq_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${cppzmq_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${cppzmq_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cppzmq_EXE_LINK_FLAGS_RELEASE}>")


set(cppzmq_COMPONENTS_RELEASE )