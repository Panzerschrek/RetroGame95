set(PROJECT_NAME RetroGame95)

cmake_minimum_required(VERSION 3.10)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
	cmake_policy(SET CMP0091 NEW)
endif()
project(${PROJECT_NAME})

# Setup language options.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# Setup compiler options.
if(MSVC)
	add_compile_options(/utf-8 /W3 /WX)
	add_definitions(
		-DNOMINMAX
		-D_CRT_SECURE_NO_WARNINGS)
	add_compile_options(/GR-)

	# Link compiler runtime statically to minimize dependencies.
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
	else()
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
	endif()

else()
	add_compile_options(-Werror -Wall -Wextra -Wconversion)
	add_compile_options(-fno-exceptions -fno-rtti)
	add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
	string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections")
	string(APPEND CMAKE_EXE_LINKER_FLAGS_MINSIZEREL " -s -Wl,--gc-sections")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

if(WIN32)
	set(SDL2_INCLUDE_DIRS ${SDL2_DIR}/include)
	set(SDL2_LIBRARIES ${SDL2_DIR}/lib/x64/SDL2.lib ${SDL2_DIR}/lib/x64/SDL2main.lib)
	set(GUI_APP_FLAG WIN32)
else()
	find_package(SDL2 REQUIRED)
	set(GUI_APP_FLAG "")
endif()

file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp")
add_executable(${PROJECT_NAME} ${GUI_APP_FLAG} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES})
