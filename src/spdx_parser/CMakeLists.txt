add_library(spdx_parser)
find_package(nlohmann_json REQUIRED)
target_sources(spdx_parser
	PRIVATE
	document.cpp
	json_util.cpp
	software.cpp
	core.cpp)

target_link_libraries(spdx_parser
	PRIVATE
	nlohmann_json::nlohmann_json)

target_include_directories(spdx_parser
	PUBLIC
	"${CMAKE_SOURCE_DIR}/include/spdx_parser"
	"${CMAKE_SOURCE_DIR}/include/common")
