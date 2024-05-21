# Tools for using NK parser.
include (platform/nk)

add_executable (KlogStorageEntity "src/klog_storage_entity.c")

target_link_libraries (KlogStorageEntity ${klog_storage_SERVER_LIB})

add_dependencies (KlogStorageEntity klog_edl_files klog_storage_edl_files)