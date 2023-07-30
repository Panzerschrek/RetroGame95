set(PROJECT_NAME Vermischung)

cmake_minimum_required(VERSION 3.10)

set(GAME_LANGUAGE "De" CACHE STRING "Language of the game. For example, En, De or nay other (with existing localization file).")
set(TARGET_EMSCRIPTEN NO CACHE BOOL "Set to yes in order to build for emscripten")
set(TARGET_DOS NO CACHE BOOL "Set to yes in order to build for MS-DOS")

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
	# Reset flags to remove exceptions handling and rtti options that are enabled by default.
	set(CMAKE_CXX_FLAGS /DWIN32 /D_WINDOWS)

	# Disable runtime buffer overrun checks.
	if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_compile_options(/GS-)
	endif()

	add_compile_options(/utf-8 /W3 /WX)
	add_definitions(
		-DNOMINMAX
		-D_CRT_SECURE_NO_WARNINGS
		-D_HAS_EXCEPTIONS=0)

	if(NOT TARGET_DOS)
		# Link compiler runtime statically to minimize dependencies.
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
		else()
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
		endif()

		# Use LTO to speed-up code and remove unused stuff.
		if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
			add_compile_options(/GL)
			add_link_options(/LTCG)
		endif()
	endif()
else()
	add_compile_options(-Werror -Wall -Wextra -Wconversion)
	add_compile_options(-fno-exceptions -fno-rtti)
	add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
	add_compile_options(-fdata-sections -ffunction-sections)
	string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections")
	string(APPEND CMAKE_EXE_LINKER_FLAGS_MINSIZEREL " -s -Wl,--gc-sections")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

# Setup dependencies.

if(WIN32)
	set(SDL2_INCLUDE_DIRS ${SDL2_DIR}/include)
	if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
		set(ARCH_DIR x64)
	else()
		set(ARCH_DIR x86)
	endif()
	set(SDL2_LIBRARIES ${SDL2_DIR}/lib/${ARCH_DIR}/SDL2.lib ${SDL2_DIR}/lib/${ARCH_DIR}/SDL2main.lib)
	set(GUI_APP_FLAG WIN32)
elseif(TARGET_EMSCRIPTEN)
	add_compile_options("-sUSE_SDL=2")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_SDL=2 --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/../emscripten/shell.html")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
	find_package(SDL2 REQUIRED)
	set(GUI_APP_FLAG "")
endif()


# Add sprites.

set(SPRITES_HEADERS_PATH ${CMAKE_CURRENT_BINARY_DIR}/sprites)
file(GLOB SPRITES "../sprites/*")
set(SPRITES_HEADER_CONTENT "")
foreach( SPRITE ${SPRITES})
	get_filename_component(SPRITE_NAME ${SPRITE} NAME_WE)
	set(HEADER_PATH ${SPRITES_HEADERS_PATH}/${SPRITE_NAME}.hpp)
	add_custom_command(
		OUTPUT ${HEADER_PATH}
		DEPENDS ${SPRITE}
		DEPENDS ${CMAKE_SOURCE_DIR}/bin2c.cmake ${SPRITE}
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/bin2c.cmake ${SPRITE} ${HEADER_PATH} ${SPRITE_NAME}
		)
	list(APPEND SPRITES_HEADERS ${HEADER_PATH})
	set(SPRITES_HEADER_CONTENT "${SPRITES_HEADER_CONTENT}#include \"${SPRITE_NAME}.hpp\"\n")
endforeach()

set(SPRITES_HEADER_CONTENT "#include <cstdint>\nnamespace Sprites\n{\n\n${SPRITES_HEADER_CONTENT}\n}\n")
set(SPRITES_HEADER "${SPRITES_HEADERS_PATH}/Sprites.hpp")
file(GENERATE OUTPUT ${SPRITES_HEADER} CONTENT "${SPRITES_HEADER_CONTENT}")

# Add music.

set(MUSIC_HEADERS_PATH ${CMAKE_CURRENT_BINARY_DIR}/music)
file(GLOB MUSIC "../music/*.mid")
set(MUSIC_HEADER_CONTENT "")
foreach(MUSIC_FILE ${MUSIC})
	get_filename_component(MUSIC_NAME ${MUSIC_FILE} NAME_WE)
	set(HEADER_PATH ${MUSIC_HEADERS_PATH}/${MUSIC_NAME}.hpp)
	add_custom_command(
		OUTPUT ${HEADER_PATH}
		DEPENDS ${MUSIC_FILE}
		DEPENDS ${CMAKE_SOURCE_DIR}/bin2c.cmake ${MUSIC_FILE}
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/bin2c.cmake ${MUSIC_FILE} ${HEADER_PATH} ${MUSIC_NAME}
		)
	list(APPEND MUSIC_HEADERS ${HEADER_PATH})
	set(MUSIC_HEADER_CONTENT "${MUSIC_HEADER_CONTENT}#include \"${MUSIC_NAME}.hpp\"\n")
endforeach()

set(MUSIC_HEADER_CONTENT "#include <cstdint>\nnamespace Music\n{\n\n${MUSIC_HEADER_CONTENT}\n}\n")
set(MUSIC_HEADER "${MUSIC_HEADERS_PATH}/Music.hpp")
file(GENERATE OUTPUT ${MUSIC_HEADER} CONTENT "${MUSIC_HEADER_CONTENT}")

# Add executable.

file(GLOB_RECURSE SOURCES "*.cpp" "*.hpp" "*.rc" "*.ico")
file(GLOB_RECURSE LOCALIZATION_SOURCES "Strings*.cpp")
list(REMOVE_ITEM SOURCES ${LOCALIZATION_SOURCES})
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Strings${GAME_LANGUAGE}.cpp)

if(TARGET_DOS)
	add_library(${PROJECT_NAME} ${SOURCES} ${SPRITES_HEADERS} ${SPRITES_HEADER} ${MUSIC_HEADERS} ${MUSIC_HEADER})
	target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SPRITES_HEADERS_PATH} ${MUSIC_HEADERS_PATH})
else()
	add_executable(${PROJECT_NAME} ${GUI_APP_FLAG} ${SOURCES} ${SPRITES_HEADERS} ${SPRITES_HEADER} ${MUSIC_HEADERS} ${MUSIC_HEADER})
	target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS} ${SPRITES_HEADERS_PATH} ${MUSIC_HEADERS_PATH})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()
