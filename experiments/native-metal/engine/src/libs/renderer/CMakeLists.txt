set(SYSTEM_DEPS "${NATIVE_RENDERER_LIB}")

STORM_SETUP(
    TARGET_NAME renderer
    TYPE storm_module
    DEPENDENCIES core render_api util ${SYSTEM_DEPS}
)

# An old ignored extraction may still contain this SDK importer. Native image
# imports are owned by image_import.cpp/texture_payload.cpp, never the glob.
get_target_property(renderer_sources renderer SOURCES)
list(FILTER renderer_sources EXCLUDE REGEX "(^|/)storm_d3dx9\\.cpp$")
set_property(TARGET renderer PROPERTY SOURCES "${renderer_sources}")
