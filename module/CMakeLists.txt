add_library(module INTERFACE)

target_include_directories_logged(module INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}")
