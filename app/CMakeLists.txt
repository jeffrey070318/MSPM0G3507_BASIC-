if(USE_FREERTOS)
    add_library(app STATIC
        robot.c
        chassis/chassis.c
        cmd/robot_cmd.c)

    target_include_directories_logged(app PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/chassis"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmd")

    target_link_libraries(app PUBLIC
        bsp
        module)

    target_compile_options(app PRIVATE
        ${MSPM0_ARCH_FLAGS}
        -O2
        -ffunction-sections
        -fdata-sections
        -g
        -gstrict-dwarf
        -Wall
        "@${SYSCFG_DEVICE_OPT}")
else()
    add_library(app INTERFACE)

    target_include_directories_logged(app INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/chassis"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmd")
endif()
