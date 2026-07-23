# Host BSP Tests

Run the hardware-independent BSP regression tests from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1
```

The tests compile the real encoder decoder, PWM, UART completion state, GPIO,
I2C, and log formatting sources against small host-side DriverLib stubs. They do
not replace on-board validation of peripheral timing and electrical behavior.
