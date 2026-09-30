cmake_minimum_required(VERSION 3.20)
project(corsairs_native_audio_probe LANGUAGES C CXX)

add_executable(native-audio-probe lifecycle_probe.cpp miniaudio_impl.cpp)
target_compile_features(native-audio-probe PRIVATE cxx_std_20)
target_link_libraries(native-audio-probe PRIVATE "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreFoundation")
