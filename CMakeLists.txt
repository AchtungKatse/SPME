cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
project(SPME)

# Compiler Settigns
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -fuse-ld=mold")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fuse-ld=mold")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ASSIMP
OPTION(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)
OPTION(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)
OPTION(ASSIMP_COVERALLS "Enable this to measure test coverage." OFF)
OPTION(ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." OFF )

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp
	)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/src/*.h
	${CMAKE_SOURCE_DIR}/src/*.hpp
	)

# Dependencies
add_subdirectory(dependencies/spdlog)
add_subdirectory(dependencies/assimp)


# Define the link libraries
message("Compiling with flags '${CMAKE_CXX_FLAGS}'")
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# Define the include DIRs
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/dependencies/assimp/include"
    "${CMAKE_SOURCE_DIR}/dependencies/spdlog/include"
    "${CMAKE_SOURCE_DIR}/dependencies/stb"
    "${CMAKE_SOURCE_DIR}/dependencies/fpng/src" # please separate your source and include folders
)

if (MSVC)
    add_definitions(/FI"PCH.h") # MSVC really didnt like me having a pch
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8")
else()
    target_precompile_headers(${PROJECT_NAME} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/PCH.h>"
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
