idf_component_register(
    SRCS
        "dps310.c"
        "hx711_driver.c"
        "acs770.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        drivers_ra
)