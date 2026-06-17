set(MSPM0_SDK_DIR "D:/tools/ti/mspm0_sdk_2_10_00_04" CACHE PATH
    "Path to the MSPM0 SDK root")
set(SYSCONFIG_CLI "D:/tools/ti/ccs/utils/sysconfig_1.27.1/sysconfig_cli.bat"
    CACHE FILEPATH "Path to sysconfig_cli.bat")
set(OPENOCD_EXE "D:/STM32_Tools/xpack-openocd/xpack-openocd-0.12.0-7/bin/openocd.exe"
    CACHE FILEPATH "Path to OpenOCD executable")
set(OPENOCD_SCRIPTS_DIR
    "D:/STM32_Tools/xpack-openocd/xpack-openocd-0.12.0-7/openocd/scripts"
    CACHE PATH "Path to OpenOCD scripts directory")

set(MSPM0_DEVICE MSPM0G3507 CACHE STRING "Target MSPM0 device")
set(MSPM0_ARCH_FLAGS
    -mcpu=cortex-m0plus
    -march=armv6-m
    -mthumb
    -mfloat-abi=soft)

set(MSPM0_SDK_PRODUCT_JSON "${MSPM0_SDK_DIR}/.metadata/product.json")
set(MSPM0_DRIVERLIB_A
    "${MSPM0_SDK_DIR}/source/ti/driverlib/lib/gcc/m0p/mspm0g1x0x_g3x0x/driverlib.a")
set(MSPM0_STARTUP_FILE
    "${MSPM0_SDK_DIR}/source/ti/devices/msp/m0p/startup_system_files/gcc/startup_mspm0g350x_gcc.c")
set(MSPM0_FREERTOS_CONFIG_DIR
    "${MSPM0_SDK_DIR}/kernel/freertos/builds/LP_MSPM0G3507/release")

foreach(_required_path
        "${MSPM0_SDK_PRODUCT_JSON}"
        "${MSPM0_DRIVERLIB_A}"
        "${MSPM0_STARTUP_FILE}"
        "${MSPM0_FREERTOS_CONFIG_DIR}/FreeRTOSConfig.h"
        "${SYSCONFIG_CLI}"
        "${OPENOCD_EXE}"
        "${OPENOCD_SCRIPTS_DIR}")
    if(NOT EXISTS "${_required_path}")
        message(FATAL_ERROR "Required MSPM0 toolchain path not found: ${_required_path}")
    endif()
endforeach()

set(MSPM0_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/syscfg"
    "${MSPM0_SDK_DIR}/source"
    "${MSPM0_SDK_DIR}/source/third_party/CMSIS/Core/Include")

set(MSPM0_FREERTOS_INCLUDE_DIRS
    "${MSPM0_FREERTOS_CONFIG_DIR}"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/include"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/portable/GCC/ARM_CM0")

set(MSPM0_FREERTOS_SOURCES
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/croutine.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/event_groups.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/list.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/queue.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/stream_buffer.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/tasks.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/timers.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/portable/MemMang/heap_4.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/portable/GCC/ARM_CM0/port.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/Source/portable/GCC/ARM_CM0/portasm.c"
    "${MSPM0_SDK_DIR}/kernel/freertos/dpl/AppHooks_freertos.c"
    "${CMAKE_SOURCE_DIR}/app/freertos_hooks.c")
