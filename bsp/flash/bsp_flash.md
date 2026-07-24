# Flash BSP

The final eight 1 KB Flash sectors (`0x0001E000` through `0x0001FFFF`) are
reserved by the linker configuration for parameter storage. Erase lengths are
sector counts; read and write lengths are 32-bit word counts.

Call erase before programming data that needs any bit to change from 0 back to
1. The current compatibility erase wrapper has no return value, while the write
functions return `0` or `-1`. Flash access should be owned by one task at a time;
wear levelling and power-loss-safe records belong in a parameter-storage layer
above this raw BSP.
