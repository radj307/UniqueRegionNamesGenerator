# ParseImage/ParseImage
cmake_minimum_required (VERSION 3.20)

file(GLOB SRCS
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIGURE_DEPENDS
	"*.c*"
)
file(GLOB HEADERS
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIGURE_DEPENDS
	"*.h*"
)

# Add source to this project's executable.
add_executable (parseimg "${SRCS}")

set_property(TARGET parseimg PROPERTY CXX_STANDARD 20)
set_property(TARGET parseimg PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET parseimg PROPERTY POSITION_INDEPENDENT_CODE ON)
if (MSVC)
	target_compile_options(parseimg PUBLIC "/Zc:__cplusplus" "/Zc:preprocessor")
endif()

target_sources(parseimg PUBLIC "${HEADERS}")

find_package(OpenCV REQUIRED)
target_include_directories(parseimg PUBLIC "${OpenCV_INCLUDE_DIRS}")

target_link_libraries(parseimg PUBLIC shared TermAPI strlib optlib filelib "${OpenCV_LIBS}")
