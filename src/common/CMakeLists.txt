add_library(common)

target_sources(common
	PRIVATE
	enums.cpp)

target_include_directories(common
	PUBLIC
	"${CMAKE_SOURCE_DIR}/include/common")
