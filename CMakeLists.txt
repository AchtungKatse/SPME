cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
project(SPME)

# Compiler Settigns
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

OPTION(BUILD_TESTS OFF)
OPTION(BUILD_VIEWER OFF)

# ASSIMP
SET(ASSIMP_BUILD_TESTS OFF)
SET(BUILD_SHARED_LIBS OFF)
SET(ASSIMP_BUILD_TESTS OFF)
SET(ASSIMP_COVERALLS OFF)
SET(ASSIMP_INSTALL OFF )
SET(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
SET(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
SET(ASSIMP_BUILD_FBX_IMPORTER ON)
SET(ASSIMP_BUILD_FBX_EXPORTER ON)
SET(ASSIMP_BUILD_GLTF_IMPORTER ON)
SET(ASSIMP_BUILD_GLTF_EXPORTER ON)

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_SOURCE_DIR}/src/Commands/*.c*
    ${CMAKE_SOURCE_DIR}/src/Compressors/*.c*
    ${CMAKE_SOURCE_DIR}/src/FileTypes/*.c*
    ${CMAKE_SOURCE_DIR}/src/IO/*.c*
    ${CMAKE_SOURCE_DIR}/src/core/*.c*
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/StbImpl.cpp
)

add_subdirectory(dependencies/assimp)
add_subdirectory(dependencies/yaml-cpp)

# Define the link libraries
message("Compiling with flags '${CMAKE_CXX_FLAGS}'")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Define the include DIRs
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/dependencies/glfw/include"
    "${CMAKE_SOURCE_DIR}/dependencies/glad/include"
    "${CMAKE_SOURCE_DIR}/dependencies/assimp/include"
    "${CMAKE_SOURCE_DIR}/dependencies/stb"
    "${CMAKE_SOURCE_DIR}/dependencies/glm"
    "${CMAKE_SOURCE_DIR}/dependencies/fpng/src" # please separate your source and include folders
    "${CMAKE_SOURCE_DIR}/dependencies/yaml-cpp/include"
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/PCH.h>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE assimp yaml-cpp)

if (BUILD_VIEWER) 

    message("Building viewer")
    add_subdirectory(dependencies/glfw)

    add_library(glad ${CMAKE_SOURCE_DIR}/dependencies/glad/src/glad.c)
    target_include_directories(glad PRIVATE "${CMAKE_SOURCE_DIR}/dependencies/glad/include")

    target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw)
else()
    message("Not building viewer")
    # target_compile_options(${PROJECT_NAME} PRIVATE SPME_NO_VIEWER)
endif()

if (${BUILD_TESTS})
    message("Building tests")
    file(GLOB_RECURSE TEST_SOURCE 
        "${CMAKE_SOURCE_DIR}/src/Commands/*.c*"
        "${CMAKE_SOURCE_DIR}/src/Compressors/*.c*"
        "${CMAKE_SOURCE_DIR}/src/FileTypes/*.c*"
        "${CMAKE_SOURCE_DIR}/src/IO/*.c*"
        "${CMAKE_SOURCE_DIR}/src/StbImpl.cpp"
        "${CMAKE_SOURCE_DIR}/src/UnitTests/*.cpp"
        "${CMAKE_SOURCE_DIR}/dependencies/glad/src/glad.c"
        "${CMAKE_SOURCE_DIR}/src/test.cpp")

    add_executable(${PROJECT_NAME}_Tests ${TEST_SOURCE})

    # Define the include DIRs
    target_include_directories(${PROJECT_NAME}_Tests PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/dependencies/glfw/include"
        "${CMAKE_SOURCE_DIR}/dependencies/glad/include"
        "${CMAKE_SOURCE_DIR}/dependencies/assimp/include"
        "${CMAKE_SOURCE_DIR}/dependencies/stb"
        "${CMAKE_SOURCE_DIR}/dependencies/glm"
        "${CMAKE_SOURCE_DIR}/dependencies/fpng/src" # please separate your source and include folders
    )

    target_precompile_headers(${PROJECT_NAME}_Tests PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/PCH.h>"
    )

    target_link_libraries(${PROJECT_NAME}_Tests PRIVATE assimp glfw glad)
endif()
