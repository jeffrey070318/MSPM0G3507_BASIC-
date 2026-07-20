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
option(BSP_LINK_CHECK
    "Force-link every BSP/module object to expose hidden link errors" OFF)

set(MSPM0_FLASH_STORAGE_PAGE_COUNT 8 CACHE STRING
    "Number of 1 KB Flash sectors reserved for parameter storage")
if(MSPM0_FLASH_STORAGE_PAGE_COUNT LESS 1 OR
        MSPM0_FLASH_STORAGE_PAGE_COUNT GREATER 127)
    message(FATAL_ERROR
        "MSPM0_FLASH_STORAGE_PAGE_COUNT must be between 1 and 127")
endif()
math(EXPR MSPM0_FLASH_STORAGE_SIZE
    "${MSPM0_FLASH_STORAGE_PAGE_COUNT} * 1024")
math(EXPR MSPM0_FLASH_STORAGE_START
    "0x00020000 - ${MSPM0_FLASH_STORAGE_SIZE}"
    OUTPUT_FORMAT HEXADECIMAL)

function(target_include_directories_logged target scope)
    foreach(include_dir ${ARGN})
        if(IS_ABSOLUTE "${include_dir}")
            set(display_include_dir "${include_dir}")
        else()
            get_filename_component(display_include_dir
                "${include_dir}" ABSOLUTE
                BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        get_property(logged_include_dirs GLOBAL PROPERTY MSPM0_LOGGED_INCLUDE_DIRECTORIES)
        if(NOT display_include_dir IN_LIST logged_include_dirs)
            message("Adding include directory: " "${display_include_dir}")
            list(APPEND logged_include_dirs "${display_include_dir}")
            set_property(GLOBAL PROPERTY MSPM0_LOGGED_INCLUDE_DIRECTORIES
                "${logged_include_dirs}")
        endif()
    endforeach()

    target_include_directories(${target} ${scope} ${ARGN})
endfunction()

set(SYSCFG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/MSPM0G3507_BASIC.syscfg")
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

file(GLOB_RECURSE BSP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.S")
list(REMOVE_ITEM BSP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp/mspm0_irq.c")

file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/module/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/module/*.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/module/*.S")

file(GLOB_RECURSE FRAMEWORK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/app/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/module/*.h")

set(FRAMEWORK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/app"
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/module")
foreach(header_file IN LISTS FRAMEWORK_HEADERS)
    get_filename_component(header_dir "${header_file}" DIRECTORY)
    list(APPEND FRAMEWORK_INCLUDE_DIRS "${header_dir}")
endforeach()
list(REMOVE_DUPLICATES FRAMEWORK_INCLUDE_DIRS)

add_library(bsp STATIC ${BSP_SOURCES})
add_dependencies(bsp syscfg)
target_include_directories_logged(bsp PUBLIC
    ${FRAMEWORK_INCLUDE_DIRS}
    ${MSPM0_INCLUDE_DIRS})
target_compile_definitions(bsp PUBLIC
    BSP_FLASH_STORAGE_START_ADDR=${MSPM0_FLASH_STORAGE_START}
    BSP_FLASH_STORAGE_PAGE_COUNT=${MSPM0_FLASH_STORAGE_PAGE_COUNT})

add_library(module STATIC ${MODULE_SOURCES})
add_dependencies(module syscfg)
target_include_directories_logged(module PUBLIC
    ${FRAMEWORK_INCLUDE_DIRS})
target_link_libraries(module PUBLIC bsp)

if(USE_FREERTOS)
    file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/app/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/*.s"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/*.S")
    list(REMOVE_ITEM APP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/app/framework_runtime.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/freertos_hooks.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/syscalls_stub.c")

    add_library(app STATIC ${APP_SOURCES})
    add_dependencies(app syscfg)
    target_include_directories_logged(app PUBLIC
        ${FRAMEWORK_INCLUDE_DIRS})
    target_link_libraries(app PUBLIC module bsp)
else()
    add_library(app INTERFACE)
    target_include_directories_logged(app INTERFACE
        ${FRAMEWORK_INCLUDE_DIRS})
endif()

set(CORE_SOURCES
    main.c
    app/framework_runtime.c
    app/syscalls_stub.c
    bsp/mspm0_irq.c
    "${SYSCFG_C_FILE}"
    "${MSPM0_STARTUP_FILE}")

if(USE_FREERTOS)
    list(APPEND CORE_SOURCES
        app/freertos_hooks.c
        ${MSPM0_FREERTOS_SOURCES})
endif()

add_executable(${PROJECT_NAME}.elf
    ${CORE_SOURCES})

add_dependencies(${PROJECT_NAME}.elf syscfg)

target_include_directories_logged(${PROJECT_NAME}.elf PRIVATE
    ${FRAMEWORK_INCLUDE_DIRS}
    ${MSPM0_INCLUDE_DIRS})

if(USE_FREERTOS)
    foreach(freertos_target bsp module app)
        target_include_directories_logged(${freertos_target} PUBLIC
            ${MSPM0_FREERTOS_INCLUDE_DIRS})
        target_compile_definitions(${freertos_target} PUBLIC
            USE_FREERTOS=1)
    endforeach()

    target_include_directories_logged(${PROJECT_NAME}.elf PRIVATE
        ${MSPM0_FREERTOS_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
        USE_FREERTOS=1)
endif()

foreach(framework_target bsp module)
    target_compile_options(${framework_target} PRIVATE
        ${MSPM0_ARCH_FLAGS}
        -O2
        -ffunction-sections
        -fdata-sections
        -g
        -gstrict-dwarf
        -Wall
        "@${SYSCFG_DEVICE_OPT}")
endforeach()

if(USE_FREERTOS)
    target_compile_options(app PRIVATE
        ${MSPM0_ARCH_FLAGS}
        -O2
        -ffunction-sections
        -fdata-sections
        -g
        -gstrict-dwarf
        -Wall
        "@${SYSCFG_DEVICE_OPT}")
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

if(BSP_LINK_CHECK)
    target_link_libraries(${PROJECT_NAME}.elf PRIVATE
        app
        "-Wl,--whole-archive"
        module
        bsp
        "-Wl,--no-whole-archive"
        "${MSPM0_DRIVERLIB_A}")
else()
    target_link_libraries(${PROJECT_NAME}.elf PRIVATE
        app
        module
        bsp
        "${MSPM0_DRIVERLIB_A}")
endif()

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MSPM0_ARCH_FLAGS}
    -nostartfiles
    -static
    -Wl,--gc-sections
    "-L${MSPM0_SDK_DIR}/source"
    "-L${MSPM0_SDK_DIR}/source/ti/driverlib/lib/gcc/m0p/mspm0g1x0x_g3x0x"
    "-T${SYSCFG_GENLIBS_LINKER}"
    "-Wl,-T,${SYSCFG_DEVICE_LINKER}"
    "-Wl,--defsym=__bsp_flash_storage_start=${MSPM0_FLASH_STORAGE_START}"
    "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/cmake/flash_storage_guard.lds"
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

  
