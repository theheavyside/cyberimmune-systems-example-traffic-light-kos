project (LightsGPIO)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable (LightsGPIO "src/lights_gpio.c")
add_dependencies (LightsGPIO
    traffic_light_lights_gpio_edl_files)
