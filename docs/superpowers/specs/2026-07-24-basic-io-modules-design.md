# Basic IO Modules Design

## Goal

Add minimal, testable module wrappers for transparent UART devices, a
relay-controlled electromagnet, and a digital photoelectric switch. The first
version establishes stable module-shaped APIs without guessing future device
protocols or adding lifecycle features that current applications do not need.

## Scope

This change adds three independent module capabilities:

- A transparent UART instance that can bind to board UART2 or UART3 and expose
  raw byte send and receive operations.
- An electromagnet instance controlled through a relay GPIO output.
- A photoelectric-switch instance read through a GPIO input with configurable
  active-high or active-low polarity.

Protocol parsing, packet framing, reconnect logic, dynamic unregistration,
ring buffers, and application-specific camera or wireless commands are outside
this version.

## Transparent UART

The public module API uses a module-level UART port enum rather than
`UART_HandleTypeDef`, `USARTInstance`, or `USART_Init_Config_s`. A device
instance is initialized for UART2 or UART3, then provides blocking raw-byte
`Send` and `Read` operations. This allows a camera and a wireless transparent
link to use the same implementation without assigning permanent roles to the
two board UARTs.

The module internally maps its port enum to the board UART handle and registers
the USART instance. The BSP gains only the generic polling receive operation
needed by UART2 and UART3. Existing UART1 DMA behavior and VOFA ownership are
unchanged.

The initial receive API returns currently available bytes up to the caller's
capacity and reports the byte count. It does not decide message boundaries.
Invalid pointers, zero capacities, unsupported ports, duplicate registration,
and use before initialization return an error status.

## Electromagnet

The electromagnet module follows the existing LED module's direct-device
pattern. Initialization accepts a GPIO port, pin, and active state because the
SysConfig signal has not been assigned yet. The module stores its registered
GPIO instance and exposes `On`, `Off`, and `Toggle` operations. Initialization
always drives the inactive state so the relay does not energize unexpectedly.

The active-state option supports relay boards that energize on either a high or
low input. No pulse timing, delayed release, or automatic safety timeout is
included.

## Photoelectric Switch

The photoelectric-switch module follows the existing key module's digital-input
pattern. Initialization accepts a GPIO port, pin, and active state, and
`IsTriggered` compares the current input with that configured state. No
debouncing, edge callback, counter, or filtering is included until hardware
behavior shows that it is required.

## Ownership And Application Use

Transparent UART hides BSP configuration and handles from application code.
The two GPIO modules temporarily accept GPIO port and pin parameters, matching
the current LED and key APIs, because the PCB signals do not yet exist in
SysConfig. Once those pins are fixed, their board mapping can move behind
parameterless or channel-based initialization without changing their runtime
operations.

Instances are initialized once and retained for the life of the firmware.
There is no unregister or deinitialization API.

## Testing

Host tests cover initialization validation, active-high and active-low GPIO
behavior, safe inactive electromagnet initialization, transparent UART port
selection, send forwarding, receive byte counts, and invalid or uninitialized
calls. Existing host tests and the normal embedded build must continue to pass.
Hardware validation remains necessary after the actual relay, photoelectric
switch, camera, and wireless devices are connected.
