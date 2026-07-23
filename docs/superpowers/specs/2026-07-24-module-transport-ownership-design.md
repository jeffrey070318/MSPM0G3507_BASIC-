# Module Transport Ownership Design

## Goal

Application code uses IMU and VOFA semantics without constructing BSP
configuration objects, registering buses, or passing BSP handles.

## IMU

The project has one JY901S. `IMU_Init()` internally registers address `0x50` on
the board MPU I2C handle. Read APIs operate on the internal singleton and expose
only IMU data types and common device status values.

## VOFA

The project has one VOFA JustFloat output. `VOFA_Init()` internally registers
the board debug UART. Output APIs accept only float data and select blocking or
DMA transmission internally. The JustFloat tail remains `00 00 80 7F`.

## Application Boundary

`app/robot.c` and the UART hardware test call module initialization and semantic
operations only. Direct BSP registration remains allowed in BSP-specific tests,
but not in normal IMU or VOFA application flows.
