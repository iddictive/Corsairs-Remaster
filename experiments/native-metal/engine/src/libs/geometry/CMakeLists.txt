STORM_SETUP(
    TARGET_NAME geometry
    TYPE storm_module
    DEPENDENCIES core renderer
)
# World-only planar winding normalization; no public Clipper dependency.
# Clipper2 2.0.1 (BSL-1.0), AngusJohnson/Clipper2 commit
# 21ebba05db8894f0c7217ad35ea518080f324946; unmodified engine/triangulation and transitive headers.
target_sources(geometry PRIVATE third_party/clipper2/src/clipper.engine.cpp third_party/clipper2/src/clipper.triangulation.cpp)
target_include_directories(geometry PRIVATE third_party/clipper2/include)
