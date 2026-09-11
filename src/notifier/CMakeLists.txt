add_library(tsnotifier
        notifier.cpp)

find_package(CURL REQUIRED)

target_include_directories(tsnotifier
        PUBLIC
        "${CMAKE_SOURCE_DIR}/include/notifier")


target_link_libraries(tsnotifier
        PUBLIC
        CURL::libcurl)
