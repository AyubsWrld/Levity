add_library(tspredicates
	"${CMAKE_CURRENT_SOURCE_DIR}/predicates.cpp")

target_include_directories(tspredicates
	PUBLIC
	"${CMAKE_SOURCE_DIR}/include/predicates"
	"${CMAKE_SOURCE_DIR}/include/common")

