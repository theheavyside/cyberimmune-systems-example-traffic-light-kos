project (Diagnostics)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
# project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (${PROJECT_NAME} "src/diagnostics.cpp")
add_dependencies (${PROJECT_NAME} traffic_light_diagnostics_edl_files traffic_light_status_idl_files)