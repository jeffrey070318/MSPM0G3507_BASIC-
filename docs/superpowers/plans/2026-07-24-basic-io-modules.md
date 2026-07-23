# Basic IO Modules Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add minimal transparent UART, relay-controlled electromagnet, and digital photoelectric-switch module wrappers.

**Architecture:** The GPIO modules follow the existing LED and key instance patterns. Transparent UART exposes its own UART2/UART3 enum and hides `UART_HandleTypeDef` and `USARTInstance`; the USART BSP receives one generic FIFO polling function while UART1 asynchronous behavior remains unchanged.

**Tech Stack:** C11, TI MSPM0 DriverLib, existing BSP registration APIs, GCC host tests, CMake/Arm GNU embedded build.

---

### Task 1: Electromagnet Module

**Files:**
- Create: `tests/electromagnet_test.c`
- Create: `module/electromagnet/electromagnet.h`
- Create: `module/electromagnet/electromagnet.c`
- Create: `module/electromagnet/electromagnet.md`
- Modify: `tests/run_host_tests.ps1`

- [ ] **Step 1: Write the failing test**

Cover active-high and active-low initialization, inactive startup, on, off,
toggle, invalid arguments, registration failure, and harmless uninitialized
operations using a host GPIO stub.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Itests/stubs -Ibsp/gpio -Imodule/electromagnet tests/electromagnet_test.c module/electromagnet/electromagnet.c -o build-host-tests/electromagnet_test.exe
```

Expected: FAIL because `electromagnet.h` and `electromagnet.c` do not exist.

- [ ] **Step 3: Write minimal implementation**

Define:

```c
typedef struct {
    GPIOInstance *gpio;
    GPIO_PinState active_state;
} Electromagnet_Device_t;

bool Electromagnet_Init(Electromagnet_Device_t *device,
    GPIO_TypeDef *gpio_port, uint32_t gpio_pin,
    GPIO_PinState active_state);
void Electromagnet_On(Electromagnet_Device_t *device);
void Electromagnet_Off(Electromagnet_Device_t *device);
void Electromagnet_Toggle(Electromagnet_Device_t *device);
```

Register one GPIO, store polarity, and force the inactive output after a
successful registration. Document that timing and automatic release belong to
the application until required otherwise.

- [ ] **Step 4: Run test to verify it passes**

Run the command from Step 2, then run the executable. Expected: PASS.

- [ ] **Step 5: Add the test to the host suite**

Add an `Invoke-HostTest "electromagnet_test"` entry with the same include paths
and sources.

### Task 2: Photoelectric Switch Module

**Files:**
- Create: `tests/photoelectric_test.c`
- Create: `module/photoelectric/photoelectric.h`
- Create: `module/photoelectric/photoelectric.c`
- Create: `module/photoelectric/photoelectric.md`
- Modify: `tests/run_host_tests.ps1`

- [ ] **Step 1: Write the failing test**

Cover active-high and active-low readings, invalid arguments, registration
failure, and uninitialized reads using a host GPIO stub.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Itests/stubs -Ibsp/gpio -Imodule/photoelectric tests/photoelectric_test.c module/photoelectric/photoelectric.c -o build-host-tests/photoelectric_test.exe
```

Expected: FAIL because the module files do not exist.

- [ ] **Step 3: Write minimal implementation**

Define:

```c
typedef struct {
    GPIOInstance *gpio;
    GPIO_PinState active_state;
} Photoelectric_Device_t;

bool Photoelectric_Init(Photoelectric_Device_t *device,
    GPIO_TypeDef *gpio_port, uint32_t gpio_pin,
    GPIO_PinState active_state);
bool Photoelectric_IsTriggered(Photoelectric_Device_t *device);
```

Register the input and compare `GPIORead()` with the configured active state.
Document that filtering and edge callbacks are intentionally absent.

- [ ] **Step 4: Run test to verify it passes**

Run the command from Step 2, then run the executable. Expected: PASS.

- [ ] **Step 5: Add the test to the host suite**

Add an `Invoke-HostTest "photoelectric_test"` entry with the same include paths
and sources.

### Task 3: Transparent UART Module

**Files:**
- Create: `tests/transparent_uart_test.c`
- Create: `module/transparent_uart/transparent_uart.h`
- Create: `module/transparent_uart/transparent_uart.c`
- Create: `module/transparent_uart/transparent_uart.md`
- Modify: `tests/module_io_stubs/bsp_usart.h`
- Modify: `bsp/usart/bsp_usart.h`
- Modify: `bsp/usart/bsp_usart.c`
- Modify: `tests/run_host_tests.ps1`

- [ ] **Step 1: Write the failing module test**

Test UART2 and UART3 mapping, registration configuration, blocking send,
receive forwarding and byte counts, unsupported port rejection, invalid
arguments, registration failure, and calls before initialization.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -Itests/module_io_stubs -Imodule/transparent_uart tests/transparent_uart_test.c module/transparent_uart/transparent_uart.c -o build-host-tests/transparent_uart_test.exe
```

Expected: FAIL because the module files and receive BSP API do not exist.

- [ ] **Step 3: Add the BSP receive API**

Declare and implement:

```c
Device_Status_e USARTReceiveAvailable(USARTInstance *instance,
    uint8_t *data, uint16_t capacity, uint16_t *received_size);
```

Validate all arguments, set `*received_size` to zero, drain bytes while the RX
FIFO is nonempty and capacity remains, then return `DEVICE_OK`. Use
`DL_UART_Main_isRXFIFOEmpty()` and `DL_UART_Main_receiveData()`. Do not alter
UART1 interrupt or DMA code.

- [ ] **Step 4: Write minimal transparent UART implementation**

Define:

```c
typedef enum {
    TRANSPARENT_UART_PORT_2 = 0,
    TRANSPARENT_UART_PORT_3,
} TransparentUART_Port_e;

typedef struct {
    TransparentUART_Port_e port;
    bool initialized;
} TransparentUART_Device_t;

Device_Status_e TransparentUART_Init(
    TransparentUART_Device_t *device, TransparentUART_Port_e port);
Device_Status_e TransparentUART_Send(
    TransparentUART_Device_t *device, uint8_t *data, uint16_t size);
Device_Status_e TransparentUART_Read(TransparentUART_Device_t *device,
    uint8_t *data, uint16_t capacity, uint16_t *received_size);
```

Internally map the enum to `huart2` or `huart3`, retain the registered USART
instance in a private two-entry table, register with a small receive buffer and
no callback, and send in blocking mode. The public device stores only the port
choice and initialization state.

- [ ] **Step 5: Run test to verify it passes**

Run the command from Step 2, then run the executable. Expected: PASS.

- [ ] **Step 6: Add the test to the host suite**

Add an `Invoke-HostTest "transparent_uart_test"` entry with the same include
paths and sources.

### Task 4: Full Verification And Documentation

**Files:**
- Modify: `tests/README.md`
- Modify: `框架使用说明.md`

- [ ] **Step 1: Run all host tests**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/run_host_tests.ps1
```

Expected: every test reports PASS.

- [ ] **Step 2: Regenerate SysConfig and build firmware**

Run:

```powershell
cmake --build build --target syscfg
cmake --build build
```

Expected: SysConfig generation and embedded linking succeed. Report SysConfig
warnings separately if present.

- [ ] **Step 3: Update documentation**

Add the three new module paths, initialization examples, UART2/UART3 selection,
GPIO polarity semantics, and the fact that real signal pins remain unassigned.

- [ ] **Step 4: Check scope and formatting**

Run:

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors; pre-existing IMU/VOFA and SysConfig changes are
preserved and remain distinguishable from this implementation.
