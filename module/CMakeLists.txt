add_library(module STATIC
    message_center/message_center.c)

target_include_directories_logged(module PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/message_center")

target_link_libraries(module PUBLIC
    bsp)

target_compile_options(module PRIVATE
    ${MSPM0_ARCH_FLAGS}
    -O2
    -ffunction-sections
    -fdata-sections
    -g
    -gstrict-dwarf
    -Wall
    "@${SYSCFG_DEVICE_OPT}")
