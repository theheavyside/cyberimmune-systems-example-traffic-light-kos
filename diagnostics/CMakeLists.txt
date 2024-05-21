project (Diagnostics)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
# project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

find_package(Boost COMPONENTS system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set (SOURCES 
    src/diagnostics.cpp
    src/tcp_server.hpp)

add_executable (${PROJECT_NAME} 
    ${SOURCES})

add_dependencies (${PROJECT_NAME} 
    traffic_light_diagnostics_edl_files 
    traffic_light_status_idl_files)

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${vfs_CLIENT_LIB})

# We do not need the default VFS entity which comes from ${vfs_CLIENT_LIB}.
set_target_properties (${PROJECT_NAME} PROPERTIES ${vfs_ENTITY}_REPLACEMENT "")
set_target_properties (${PROJECT_NAME} PROPERTIES LINK_FLAGS ${VULN_LDFLAGS})