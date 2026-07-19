add_library(bsp STATIC
    bsp_init.c
    bsp_device.c
    mspm0_irq.c
    bsp_tools.c
    adc/bsp_adc.c
    capture/bsp_capture.c
    dwt/bsp_dwt.c
    encoder/bsp_encoder.c
    flash/bsp_flash.c
    gpio/bsp_gpio.c
    iic/bsp_iic.c
    log/bsp_log.c
    pwm/bsp_pwm.c
    spi/bsp_spi.c
    timer/bsp_timer.c
    usart/bsp_usart.c)

add_dependencies(bsp syscfg)

target_include_directories_logged(bsp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/adc"
    "${CMAKE_CURRENT_SOURCE_DIR}/capture"
    "${CMAKE_CURRENT_SOURCE_DIR}/dwt"
    "${CMAKE_CURRENT_SOURCE_DIR}/encoder"
    "${CMAKE_CURRENT_SOURCE_DIR}/flash"
    "${CMAKE_CURRENT_SOURCE_DIR}/gpio"
    "${CMAKE_CURRENT_SOURCE_DIR}/iic"
    "${CMAKE_CURRENT_SOURCE_DIR}/log"
    "${CMAKE_CURRENT_SOURCE_DIR}/pwm"
    "${CMAKE_CURRENT_SOURCE_DIR}/spi"
    "${CMAKE_CURRENT_SOURCE_DIR}/timer"
    "${CMAKE_CURRENT_SOURCE_DIR}/usart"
    "${SYSCFG_OUTPUT_DIR}"
    ${MSPM0_INCLUDE_DIRS})

target_compile_definitions(bsp PUBLIC
    BSP_FLASH_STORAGE_START_ADDR=${MSPM0_FLASH_STORAGE_START}
    BSP_FLASH_STORAGE_PAGE_COUNT=${MSPM0_FLASH_STORAGE_PAGE_COUNT})

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
