if(DEFINED STORM_ENABLE_AUDIO AND NOT STORM_ENABLE_AUDIO)
    add_library(sound_service INTERFACE)
    target_include_directories(sound_service INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    return()
endif()

if(DEFINED STORM_NATIVE_AUDIO_SOURCE)
    add_library(sound_service STATIC
        "${STORM_NATIVE_AUDIO_SOURCE}/sound_service.cpp"
        "${STORM_NATIVE_AUDIO_SOURCE}/miniaudio_impl.cpp")
    target_include_directories(sound_service PUBLIC
        "${STORM_NATIVE_AUDIO_SOURCE}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(sound_service PUBLIC core renderer
        "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreFoundation")
    set(global_modules_list "${global_modules_list};sound_service" CACHE INTERNAL "global_modules_list")
    return()
endif()

STORM_SETUP(
    TARGET_NAME sound_service
    TYPE storm_module
    DEPENDENCIES core fmod renderer
)
