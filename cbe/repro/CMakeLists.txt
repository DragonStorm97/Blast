# cbe's reproductions, built from this checkout's own sources -- see README.md.
cmake_minimum_required(VERSION 3.20)
project(blast_cbe_repro LANGUAGES CXX)

set(root "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_executable(repro
    repro.cpp
    ${root}/source/sdk/common/NvBlastAssert.cpp
    ${root}/source/sdk/common/NvBlastAtomic.cpp
    ${root}/source/sdk/common/NvBlastTime.cpp
    ${root}/source/sdk/common/NvBlastTimers.cpp
    ${root}/source/sdk/lowlevel/NvBlastActor.cpp
    ${root}/source/sdk/lowlevel/NvBlastActorSerializationBlock.cpp
    ${root}/source/sdk/lowlevel/NvBlastAsset.cpp
    ${root}/source/sdk/lowlevel/NvBlastAssetHelper.cpp
    ${root}/source/sdk/lowlevel/NvBlastFamily.cpp
    ${root}/source/sdk/lowlevel/NvBlastFamilyGraph.cpp
    ${root}/source/sdk/globals/NvBlastGlobals.cpp
    ${root}/source/sdk/globals/NvBlastInternalProfiler.cpp
    ${root}/source/sdk/extensions/stress/NvBlastExtStressSolver.cpp
    ${root}/source/shared/stress_solver/stress.cpp
)

target_include_directories(repro PRIVATE
    ${root}/include
    ${root}/include/lowlevel
    ${root}/include/globals
    ${root}/include/shared/NvFoundation
    ${root}/include/extensions/stress
    ${root}/source/sdk/common
    ${root}/source/sdk/lowlevel
    ${root}/source/sdk/globals
    ${root}/source/shared/NsFoundation/include
    ${root}/source/sdk/extensions/stress
    ${root}/source/shared/stress_solver
)

# NvPreprocessor.h needs exactly one of NDEBUG and _DEBUG; the release CRT, whatever the configuration.
target_compile_definitions(repro PRIVATE NDEBUG _CRT_SECURE_NO_WARNINGS)
set_target_properties(repro PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON
    MSVC_RUNTIME_LIBRARY "MultiThreaded")
if(MSVC)
    target_compile_options(repro PRIVATE /w /EHsc /bigobj)
else()
    target_compile_options(repro PRIVATE -w)
endif()
