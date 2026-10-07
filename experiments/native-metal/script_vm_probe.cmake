# Include beside engine: imported dependency aliases belong to that directory.
add_executable(script-vm-probe EXCLUDE_FROM_ALL "${METAL_PROJECT_ROOT}/experiments/native-metal/script_vm_probe.cpp")
target_include_directories(script-vm-probe PRIVATE "${STORM_SOURCE}/src/libs/core/src" $<TARGET_PROPERTY:core,INCLUDE_DIRECTORIES>)
target_compile_definitions(script-vm-probe PRIVATE $<TARGET_PROPERTY:engine,COMPILE_DEFINITIONS>)
target_compile_options(script-vm-probe PRIVATE $<TARGET_PROPERTY:engine,COMPILE_OPTIONS>)
target_link_libraries(script-vm-probe PRIVATE $<TARGET_PROPERTY:engine,LINK_LIBRARIES>)
target_link_options(script-vm-probe PRIVATE $<TARGET_PROPERTY:engine,LINK_OPTIONS>)
set_target_properties(script-vm-probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
