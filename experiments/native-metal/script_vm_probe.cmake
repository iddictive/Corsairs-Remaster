# Include beside engine: imported dependency aliases and module registrations
# belong to that directory. Diagnostics reuse the actual engine linkage.
foreach(diagnostic IN ITEMS script-vm-probe sea-fleet-benchmark)
  string(REPLACE "-" "_" source "${diagnostic}")
  add_executable(${diagnostic} EXCLUDE_FROM_ALL "${METAL_PROJECT_ROOT}/experiments/native-metal/${source}.cpp")
  target_include_directories(${diagnostic} PRIVATE "${STORM_SOURCE}/src/libs/core/src" $<TARGET_PROPERTY:core,INCLUDE_DIRECTORIES>)
  target_compile_definitions(${diagnostic} PRIVATE $<TARGET_PROPERTY:engine,COMPILE_DEFINITIONS>)
  target_compile_options(${diagnostic} PRIVATE $<TARGET_PROPERTY:engine,COMPILE_OPTIONS>)
  target_link_libraries(${diagnostic} PRIVATE $<TARGET_PROPERTY:engine,LINK_LIBRARIES>)
  target_link_options(${diagnostic} PRIVATE $<TARGET_PROPERTY:engine,LINK_OPTIONS>)
  set_target_properties(${diagnostic} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()
target_include_directories(sea-fleet-benchmark PRIVATE $<TARGET_PROPERTY:renderer,INCLUDE_DIRECTORIES>)
