if (${UNIT_TESTS})
    add_subdirectory(unit)
endif ()

if (${FUNCTIONAL_TESTS})
    add_subdirectory(functional)
endif ()

