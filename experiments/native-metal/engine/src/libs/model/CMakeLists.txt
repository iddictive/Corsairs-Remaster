if(WIN32)
    set(MODEL_TEST_DEPENDENCIES comctl32)
endif()

STORM_SETUP(
    TARGET_NAME model
    TYPE storm_module
    DEPENDENCIES animation collide core geometry renderer
    TEST_DEPENDENCIES ${MODEL_TEST_DEPENDENCIES}
)
