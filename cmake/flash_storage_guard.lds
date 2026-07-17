ASSERT((__bsp_flash_storage_start % 1024) == 0,
    "BSP Flash storage start must be sector aligned")

ASSERT((LOADADDR(.data) + SIZEOF(.data)) <= __bsp_flash_storage_start,
    "Firmware image overlaps the reserved BSP Flash storage area")
