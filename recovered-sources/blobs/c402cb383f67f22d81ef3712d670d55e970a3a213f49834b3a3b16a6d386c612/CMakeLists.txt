add_definitions(-DUNICODE -D_UNICODE) # Use Unicode
add_definitions(-DNOMINMAX) # Use std::max()/std::min()
add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Disable deprecation of unsafe functions

if (STORM_ENABLE_STEAM)
    add_definitions(-DSTORM_ENABLE_STEAM=1)
endif()

if(STORM_ENABLE_CRASH_REPORTS)
	add_definitions(-DSTORM_ENABLE_CRASH_REPORTS=1)
endif()

if(STORM_WATERMARK_FILE)
  set(STORM_GENERATED_WATERMARK_FILE "${CMAKE_CURRENT_BINARY_DIR}/storm_build_watermark.hpp")
  configure_file("${STORM_WATERMARK_FILE}" "${STORM_GENERATED_WATERMARK_FILE}" COPYONLY)
  include_directories("${CMAKE_CURRENT_BINARY_DIR}")
  add_definitions(-DSTORM_WATERMARK_FILE="storm_build_watermark.hpp")
endif()

if(STORM_ENABLE_SAFE_MODE)
  add_definitions(-DSTORM_ENABLE_SAFE_MODE=1)
endif()

if (MSVC)
    # Ignore warnings about missing pdb
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")

    # Keep compile-time debug information embedded when requested by the
    # toolchain. This avoids the shared mspdbsrv process used by /Zi, which is
    # unreliable under Wine, while /DEBUG still produces the final linker PDB.
    if(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT STREQUAL "Embedded")
        add_compile_options(/Z7)
    else()
        add_compile_options(/Zi)
    endif()
    add_link_options(/DEBUG)

    add_compile_options(/W2) # Warning level
    add_compile_options(/WX) # Treats compiler warnings as errors
    add_compile_options(/std:c++latest) # Language Standard Version
    add_compile_options(/MP) # Build with Multiple Processes
    add_compile_options(/fp:fast) # Build with Multiple Processes


    add_compile_options("$<$<CONFIG:Release>:/Gy>") # Enable Function-Level Linking
    add_compile_options("$<$<CONFIG:Release>:/Oi>") # Generate intrinsic functions
    add_compile_options("$<$<CONFIG:Release>:/Ot>") # Favor fast code, should be implied by /O2 though
    add_compile_options("$<$<CONFIG:Release>:/GL>") # Whole program optimization

    add_link_options("$<$<CONFIG:Release>:/OPT:REF>") # Eliminate functions and data that are never referenced, needs to be explicitly specified with /DEBUG
    add_link_options("$<$<CONFIG:Release>:/OPT:ICF>") # Perform identical COMDAT folding, needs to be explicitly specified with /DEBUG
    add_link_options("$<$<CONFIG:Release>:/LTCG>") # Enable link-time code generation
else()
    add_compile_options(-std=c++20)

    # Verbose output
    add_compile_options(-v)
    add_link_options(-v)

    set(CMAKE_CXX_FLAGS_RELEASE "-O1 -g -DNDEBUG -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")

    # Add _DEBUG flag
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif()

# this is crucial to include libs first
add_subdirectory(libs)
add_subdirectory(apps)
