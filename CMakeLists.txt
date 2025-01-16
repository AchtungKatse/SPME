cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
project(SPME)

# Compiler Settigns
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -fuse-ld=mold")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fuse-ld=mold")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Assimp
# Yeah we dont need most of this code
option(ASSIMP_BUILD_3DS_IMPORTER off)
option(ASSIMP_BUILD_AC_IMPORTER off)
option(ASSIMP_BUILD_ASE_IMPORTER off)
option(ASSIMP_BUILD_ASSBIN_IMPORTER off)
option(ASSIMP_BUILD_B3D_IMPORTER off)
option(ASSIMP_BUILD_BVH_IMPORTER off)
option(ASSIMP_BUILD_COLLADA_IMPORTER off)
option(ASSIMP_BUILD_DXF_IMPORTER off)
option(ASSIMP_BUILD_CSM_IMPORTER off)
option(ASSIMP_BUILD_HMP_IMPORTER off)
option(ASSIMP_BUILD_IRRMESH_IMPORTER off)
option(ASSIMP_BUILD_IQM_IMPORTER off)
option(ASSIMP_BUILD_IRR_IMPORTER off)
option(ASSIMP_BUILD_LWO_IMPORTER off)
option(ASSIMP_BUILD_LWS_IMPORTER off)
option(ASSIMP_BUILD_M3D_IMPORTER off)
option(ASSIMP_BUILD_MD2_IMPORTER off)
option(ASSIMP_BUILD_MD3_IMPORTER off)
option(ASSIMP_BUILD_MD5_IMPORTER off)
option(ASSIMP_BUILD_MDC_IMPORTER off)
option(ASSIMP_BUILD_MDL_IMPORTER off)
option(ASSIMP_BUILD_NFF_IMPORTER off)
option(ASSIMP_BUILD_NDO_IMPORTER off)
option(ASSIMP_BUILD_OFF_IMPORTER off)
option(ASSIMP_BUILD_OBJ_IMPORTER off)
option(ASSIMP_BUILD_OGRE_IMPORTER off)
option(ASSIMP_BUILD_OPENGEX_IMPORTER off)
option(ASSIMP_BUILD_PLY_IMPORTER off)
option(ASSIMP_BUILD_MS3D_IMPORTER off)
option(ASSIMP_BUILD_COB_IMPORTER off)
option(ASSIMP_BUILD_BLEND_IMPORTER off)
option(ASSIMP_BUILD_IFC_IMPORTER off)
option(ASSIMP_BUILD_XGL_IMPORTER off)
option(ASSIMP_BUILD_FBX_IMPORTER off)
option(ASSIMP_BUILD_Q3D_IMPORTER off)
option(ASSIMP_BUILD_Q3BSP_IMPORTER off)
option(ASSIMP_BUILD_RAW_IMPORTER off)
option(ASSIMP_BUILD_SIB_IMPORTER off)
option(ASSIMP_BUILD_SMD_IMPORTER off)
option(ASSIMP_BUILD_STL_IMPORTER off)
option(ASSIMP_BUILD_TERRAGEN_IMPORTER off)
option(ASSIMP_BUILD_3D_IMPORTER off)
option(ASSIMP_BUILD_X_IMPORTER off)
option(ASSIMP_BUILD_X3D_IMPORTER off)
option(ASSIMP_BUILD_GLTF_IMPORTER off)
option(ASSIMP_BUILD_3MF_IMPORTER off)
option(ASSIMP_BUILD_MMD_IMPORTER off)
option(ASSIMP_BUILD_OBJ_EXPORTER off)
option(ASSIMP_BUILD_OPENGEX_EXPORTER off)
option(ASSIMP_BUILD_PLY_EXPORTER off)
option(ASSIMP_BUILD_3DS_EXPORTER off)
option(ASSIMP_BUILD_ASSBIN_EXPORTER off)
option(ASSIMP_BUILD_ASSXML_EXPORTER off)
option(ASSIMP_BUILD_M3D_EXPORTER off)
option(ASSIMP_BUILD_COLLADA_EXPORTER off)
option(ASSIMP_BUILD_STL_EXPORTER off)
option(ASSIMP_BUILD_X X3D_EXPORTER off)
option(ASSIMP_BUILD_GLTF_EXPORTER off)
option(ASSIMP_BUILD_M3MF_EXPORTER off)
option(ASSIMP_BUILD_MPBRT_EXPORTER off)
option(ASSIMP_BUILD_MASSJSON_EXPORTER off)
option(ASSIMP_BUILD_MSTEP_EXPORTER off)
OPTION( BUILD_SHARED_LIBS "Build package with shared libraries." OFF)
OPTION ( ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)
OPTION ( ASSIMP_COVERALLS "Enable this to measure test coverage." OFF)
OPTION( ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." OFF )
OPTION( ASSIMP_INJECT_DEBUG_POSTFIX "Inject debug postfix in .a/.so/.dll lib names" ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp
    # ${CMAKE_SOURCE_DIR}/dependencies/fpng/src/fpng.cpp # fpng is weird
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
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# Define the include DIRs
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/dependencies/assimp/include"
    "${CMAKE_SOURCE_DIR}/dependencies/spdlog/include"
    "${CMAKE_SOURCE_DIR}/dependencies/stb"
    "${CMAKE_SOURCE_DIR}/dependencies/fpng/src" # please separate your source and include folders. In heindsight I didnt even do this myself
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/PCH.h>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
