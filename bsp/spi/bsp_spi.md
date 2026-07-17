# SPI

The current project uses `SPI_0_INST` in Mode 0 at 8 MHz. SysConfig owns the
controller pins and timing. Chip select is a normal GPIO controlled by the BSP.

```c
SPI_Init_Config_s config = {
    .spi_handle = &hspi1,
    .GPIOx = GPIOB,
    .cs_pin = GPIO_PIN_14,
    .spi_work_mode = SPI_BLOCK_MODE,
};

SPIInstance *device = SPIRegister(&config);
```

The current implementation uses polling transfers with FIFO, timeout and bus
ownership checks. DMA and interrupt modes are not configured yet.
