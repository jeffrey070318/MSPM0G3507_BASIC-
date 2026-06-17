cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi-gcc.cmake"
        CACHE FILEPATH "CMake toolchain file")
endif()

project(MSPM0G3507_BASIC LANGUAGES C ASM)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/mspm0g3507.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(USE_FREERTOS "Build with the MSPM0 SDK FreeRTOS kernel" ON)

set(SYSCFG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/empty.syscfg")
set(SYSCFG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/syscfg")
set(SYSCFG_C_FILE "${SYSCFG_OUTPUT_DIR}/ti_msp_dl_config.c")
set(SYSCFG_H_FILE "${SYSCFG_OUTPUT_DIR}/ti_msp_dl_config.h")
set(SYSCFG_DEVICE_OPT "${SYSCFG_OUTPUT_DIR}/device.opt")
set(SYSCFG_DEVICE_LINKER "${SYSCFG_OUTPUT_DIR}/device_linker.lds")
set(SYSCFG_GENLIBS_LINKER "${SYSCFG_OUTPUT_DIR}/device.lds.genlibs")

add_custom_command(
    OUTPUT
        "${SYSCFG_C_FILE}"
        "${SYSCFG_H_FILE}"
        "${SYSCFG_DEVICE_OPT}"
        "${SYSCFG_DEVICE_LINKER}"
        "${SYSCFG_GENLIBS_LINKER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SYSCFG_OUTPUT_DIR}"
    COMMAND "${SYSCONFIG_CLI}"
        --compiler gcc
        --product "${MSPM0_SDK_PRODUCT_JSON}"
        --output "${SYSCFG_OUTPUT_DIR}"
        "${SYSCFG_SOURCE}"
    DEPENDS "${SYSCFG_SOURCE}"
    COMMENT "Generating MSPM0 SysConfig files"
    VERBATIM)

add_custom_target(syscfg DEPENDS
    "${SYSCFG_C_FILE}"
    "${SYSCFG_H_FILE}"
    "${SYSCFG_DEVICE_OPT}"
    "${SYSCFG_DEVICE_LINKER}"
    "${SYSCFG_GENLIBS_LINKER}")

add_subdirectory(bsp)
add_subdirectory(module)
add_subdirectory(app)

set(APP_SOURCES
    empty.c
    app/syscalls_stub.c
    "${SYSCFG_C_FILE}"
    "${MSPM0_STARTUP_FILE}")

if(USE_FREERTOS)
    list(APPEND APP_SOURCES ${MSPM0_FREERTOS_SOURCES})
endif()

add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES})

add_dependencies(${PROJECT_NAME}.elf syscfg)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${MSPM0_INCLUDE_DIRS})

if(USE_FREERTOS)
    target_include_directories(${PROJECT_NAME}.elf PRIVATE
        ${MSPM0_FREERTOS_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
        USE_FREERTOS=1)
endif()

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MSPM0_ARCH_FLAGS}
    -O2
    -ffunction-sections
    -fdata-sections
    -g
    -gstrict-dwarf
    -Wall
    "@${SYSCFG_DEVICE_OPT}")

target_link_libraries(${PROJECT_NAME}.elf PRIVATE
    app
    module
    bsp
    "${MSPM0_DRIVERLIB_A}")

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MSPM0_ARCH_FLAGS}
    -nostartfiles
    -static
    -Wl,--gc-sections
    "-L${MSPM0_SDK_DIR}/source"
    "-L${MSPM0_SDK_DIR}/source/ti/driverlib/lib/gcc/m0p/mspm0g1x0x_g3x0x"
    "-T${SYSCFG_GENLIBS_LINKER}"
    "-Wl,-T,${SYSCFG_DEVICE_LINKER}"
    "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map"
    --specs=nano.specs
    --specs=nosys.specs)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND "${CMAKE_SIZE}" $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMAND "${CMAKE_OBJCOPY}" -O ihex
        $<TARGET_FILE:${PROJECT_NAME}.elf>
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary
        $<TARGET_FILE:${PROJECT_NAME}.elf>
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    VERBATIM)

add_custom_target(probe
    COMMAND "${OPENOCD_EXE}"
        -s "${OPENOCD_SCRIPTS_DIR}"
        -f "${CMAKE_CURRENT_SOURCE_DIR}/openocd/cmsis_dap_mspm0g3507.cfg"
        -c "init"
        -c "targets"
        -c "shutdown"
    USES_TERMINAL
    COMMENT "Probing MSPM0G3507 via CMSIS-DAP")

add_custom_target(flash
    COMMAND "${OPENOCD_EXE}"
        -s "${OPENOCD_SCRIPTS_DIR}"
        -f "${CMAKE_CURRENT_SOURCE_DIR}/openocd/cmsis_dap_mspm0g3507.cfg"
        -c "program $<TARGET_FILE:${PROJECT_NAME}.elf> verify reset exit"
    DEPENDS ${PROJECT_NAME}.elf
    USES_TERMINAL
    COMMENT "Flashing MSPM0G3507 via CMSIS-DAP")

add_custom_target(erase
    COMMAND "${OPENOCD_EXE}"
        -s "${OPENOCD_SCRIPTS_DIR}"
        -f "${CMAKE_CURRENT_SOURCE_DIR}/openocd/cmsis_dap_mspm0g3507.cfg"
        -c "init"
        -c "mspm0_mass_erase"
        -c "shutdown"
    USES_TERMINAL
    COMMENT "Mass erasing MSPM0G3507 via CMSIS-DAP")

  
