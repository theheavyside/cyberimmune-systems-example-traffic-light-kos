cmake_minimum_required (VERSION 3.12)

project (traffic_light)

# Initialize CMake library for KasperskyOS SDK.
include (platform)
initialize_platform (FORCE_STATIC)

# Tools for using NK parser.
include (platform/nk)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# NK targets
# IMode / CMode
nk_build_idl_files (traffic_light_mode_idl_files 
    NK_MODULE "traffic_light" 
    IDL "resources/interfaces/IMode.idl")

nk_build_cdl_files (traffic_light_mode_cdl_files 
    IDL_TARGET traffic_light_mode_idl_files 
    NK_MODULE "traffic_light" 
    CDL "resources/interfaces/CMode.cdl")

# IStatus / CStatus
nk_build_idl_files (traffic_light_status_idl_files 
    NK_MODULE "traffic_light" 
    IDL "resources/interfaces/IStatus.idl")

nk_build_cdl_files (traffic_light_status_cdl_files 
    IDL_TARGET traffic_light_status_idl_files 
    NK_MODULE "traffic_light" 
    CDL "resources/interfaces/CStatus.cdl")

# ControlSystem
nk_build_edl_files (traffic_light_control_system_edl_files 
    NK_MODULE "traffic_light" 
    EDL "resources/entities/ControlSystem.edl")

# LightsGPIO
nk_build_edl_files (traffic_light_lights_gpio_edl_files 
    CDL_TARGET traffic_light_mode_cdl_files
    NK_MODULE "traffic_light" 
    EDL "resources/entities/LightsGPIO.edl")

nk_build_edl_files (traffic_light_lights_gpio_edl_files 
    CDL_TARGET traffic_light_status_cdl_files 
    NK_MODULE "traffic_light" 
    EDL "resources/entities/LightsGPIO.edl")
    
# Diagnostics
nk_build_edl_files (traffic_light_diagnostics_edl_files 
    NK_MODULE "traffic_light" 
    EDL "resources/entities/Diagnostics.edl")

# Add a package for working with the virtual file system.
find_package (vfs REQUIRED)
include_directories (${vfs_INCLUDE})

# Add a package with the VFS program implementations.
find_package (precompiled_vfs REQUIRED)
include_directories (${precompiled_vfs_INCLUDE})

# Recommended compiler flags against vulnerabilities.
set(VULN_LDFLAGS "\
    -Wl,-z,noexecstack \
    -Wl,-z,separate-code \
    -Wl,-z,now")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall -Wextra -Wconversion \
    -fPIE -pie -D_FORTIFY_SOURCE=2 -O2 \
    -fstack-protector-strong -fstack-clash-protection \
    -mbranch-protection=standard \
    -Wsign-conversion -Wformat=2 -Wformat-security -Werror=format-security \
    -fsanitize=undefined -fsanitize-undefined-trap-on-error")

add_subdirectory (control_system)
add_subdirectory (lights_gpio)
add_subdirectory (diagnostics)
add_subdirectory (einit)
