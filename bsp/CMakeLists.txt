add_library(bsp STATIC
    bsp_init.c
    bsp_mspm0_compat.c
    bsp_tools.c
    adc/bsp_adc.c
    dwt/bsp_dwt.c
    flash/bsp_flash.c
    gpio/bsp_gpio.c
    iic/bsp_iic.c
    log/bsp_log.c
    pwm/bsp_pwm.c
    spi/bsp_spi.c
    usart/bsp_usart.c)

add_dependencies(bsp syscfg)

target_include_directories_logged(bsp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/adc"
    "${CMAKE_CURRENT_SOURCE_DIR}/dwt"
    "${CMAKE_CURRENT_SOURCE_DIR}/flash"
    "${CMAKE_CURRENT_SOURCE_DIR}/gpio"
    "${CMAKE_CURRENT_SOURCE_DIR}/iic"
    "${CMAKE_CURRENT_SOURCE_DIR}/log"
    "${CMAKE_CURRENT_SOURCE_DIR}/pwm"
    "${CMAKE_CURRENT_SOURCE_DIR}/spi"
    "${CMAKE_CURRENT_SOURCE_DIR}/usart"
    "${CMAKE_CURRENT_SOURCE_DIR}/usb"
    "${SYSCFG_OUTPUT_DIR}"
    ${MSPM0_INCLUDE_DIRS})

if(USE_FREERTOS)
    target_include_directories_logged(bsp PUBLIC
        ${MSPM0_FREERTOS_INCLUDE_DIRS})
    target_compile_definitions(bsp PUBLIC
        USE_FREERTOS=1)
endif()

target_compile_options(bsp PRIVATE
    ${MSPM0_ARCH_FLAGS}
    -O2
    -ffunction-sections
    -fdata-sections
    -g
    -gstrict-dwarf
    -Wall
    "@${SYSCFG_DEVICE_OPT}")
