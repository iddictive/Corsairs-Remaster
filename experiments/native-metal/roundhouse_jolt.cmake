include(FetchContent)

# CPU-only, static Jolt. Keep the embedding engine's compiler flags and runtime.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static dependencies" FORCE)
set(JPH_BUILD_SHARED_LIBS OFF CACHE BOOL "Build static Jolt" FORCE)
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "Preserve engine compiler flags" FORCE)
set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "No dependency debug payload" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "No dependency LTO" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "No dependency install target" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "Use embedding project warning policy" FORCE)
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "No Jolt debug renderer" FORCE)
set(DEBUG_RENDERER_IN_DISTRIBUTION OFF CACHE BOOL "No Jolt debug renderer" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "No Jolt profiler" FORCE)
set(PROFILER_IN_DISTRIBUTION OFF CACHE BOOL "No Jolt profiler" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "No serialized Jolt assets" FORCE)
set(JPH_USE_DX12 OFF CACHE BOOL "CPU ragdolls only" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "CPU ragdolls only" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "CPU ragdolls only" FORCE)
set(JPH_USE_CPU_COMPUTE OFF CACHE BOOL "No hair compute backend" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "No Jolt examples" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "No Jolt benchmarks" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "No Jolt tests" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "No Jolt samples" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "No Jolt viewer" FORCE)

FetchContent_Declare(roundhouse_jolt
  URL "https://codeload.github.com/jrouwe/JoltPhysics/tar.gz/refs/tags/v5.6.0"
  URL_HASH "SHA256=6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e"
  SOURCE_SUBDIR Build
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(roundhouse_jolt)

add_library(roundhouse_ragdoll STATIC "${CMAKE_CURRENT_LIST_DIR}/roundhouse_ragdoll.cpp")
target_include_directories(roundhouse_ragdoll PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}/engine/src/libs/math/include"
  "${CMAKE_CURRENT_LIST_DIR}/engine/src/libs/shared_headers/include")
target_link_libraries(roundhouse_ragdoll PRIVATE Jolt)
