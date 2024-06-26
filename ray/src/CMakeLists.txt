cmake_minimum_required(VERSION 3.16)
project(utcs_ray)

set(OpenGL_GL_PREFERENCE "LEGACY")
SET(pwd ${CMAKE_CURRENT_LIST_DIR})
UNSET(src)

# Uncomment the following lines to explicitly set files to compile from
# CMakeLists.sources
# IF (EXISTS CMakeLists.sources)
# FILE(STRINGS CMakeLists.sources src)
# ENDIF()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Wall -Wextra -Wno-unknown-pragmas)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Wall -Wextra -Wno-unknown-pragmas)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/W3)
endif()

# By default, source files are added automatically
IF(NOT src)
	AUX_SOURCE_DIRECTORY(${pwd} src)
	AUX_SOURCE_DIRECTORY(${pwd}/fileio src)
	AUX_SOURCE_DIRECTORY(${pwd}/parser src)
	AUX_SOURCE_DIRECTORY(${pwd}/scene src)
	AUX_SOURCE_DIRECTORY(${pwd}/SceneObjects src)
	AUX_SOURCE_DIRECTORY(${pwd}/ui src)

	IF(WIN32)
		AUX_SOURCE_DIRECTORY(${pwd}/win32 src)
	ENDIF(WIN32)
ENDIF(NOT src)

add_executable(ray ${src})

message(STATUS "ray added, files ${src}")

target_link_libraries(ray ${OPENGL_gl_LIBRARY})
SET(FLTK_SKIP_FLUID TRUE)
FIND_PACKAGE(FLTK REQUIRED)
SET_PROPERTY(TARGET ray APPEND PROPERTY INCLUDE_DIRECTORIES ${FLTK_INCLUDE_DIRS})
SET_PROPERTY(TARGET ray APPEND PROPERTY INCLUDE_DIRECTORIES ${FLTK_INCLUDE_DIR})

# if(WIN32)
# set(FLTK_LIBRARIES fltk;fltk_gl)
# endif()
target_include_directories(ray SYSTEM PUBLIC ${pwd}/libs)

target_link_libraries(ray ${FLTK_LIBRARIES})

FIND_PACKAGE(PNG REQUIRED)
target_link_libraries(ray ${PNG_LIBRARIES})
FIND_PACKAGE(ZLIB REQUIRED)
target_link_libraries(ray ${ZLIB_LIBRARIES})
SET_PROPERTY(TARGET ray APPEND PROPERTY INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIR})
target_link_libraries(ray ${OPENGL_glu_LIBRARY})

target_include_directories(ray SYSTEM PUBLIC ${pwd}/libs)

SET_PROPERTY(TARGET ray PROPERTY CXX_STANDARD 17)
