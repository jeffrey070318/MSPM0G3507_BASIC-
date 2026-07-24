# INS Return-To-Origin Module

> 注意：本模块来自 `chassis-dev` 的惯导返航逻辑，目前只完成接口适配和宿主测试，尚未经过实车测试、参数整定和安全修正，不能直接用于无人看护运行。

The module combines JY901S yaw and left/right encoder totals for two-dimensional
dead reckoning. `INS_StartReturn()` commands a differential chassis back toward
the recorded origin through the message center.

The wheel radius, encoder PPR, gear ratio, arrival threshold, and both PID
parameter sets are template values. Confirm encoder directions, count scale,
vehicle dimensions, IMU yaw direction, motor signs, and emergency-stop behavior
before enabling the application.

Define both `ROBOT_ENABLE_CHASSIS_APP` and `ROBOT_ENABLE_INS_APP` in
`app/robot_def.h` only when the hardware is ready. With INS disabled, the
existing chassis test command path remains unchanged.
