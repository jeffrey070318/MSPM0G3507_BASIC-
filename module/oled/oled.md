# OLED

OLED module 使用 `OLED_I2C` 和 7 位地址 `0x3C` 驱动 128x64 SSD1306。模块维护本地 GRAM，调用 `OLED_printf()` 写入内容，最后通过 `OLED_refresh_gram()` 全屏刷新。

## 正常运行

正常模式由 `robot_task.h` 创建低优先级 `oledtask`，每 200 ms 调用一次 `RobotOLEDTask()`。OLED 初始化在调度器启动后完成，失败时下周期重试，不会阻止 Robot 或 chassis 初始化。

当前五行内容：

1. 两路 Motor 实际使能状态和手动命令状态；
2. 左轮目标速度与实测速度，单位 `counts/s`；
3. 右轮目标速度与实测速度，单位 `counts/s`；
4. 左右控制输出的千分比；
5. 手动 `vx` 和 `wz`，单位 `mm/s` 与 `mrad/s`。

Live Watch 可观察 `robot_oled_initialized`、`robot_oled_refresh_count` 和 `robot_oled_error_count`。

OLED 全屏刷新是阻塞 I2C 操作，因此不能放进 5 ms 的 `RobotTask()` 或 Motor 控制循环。硬件测试模式不会创建正常 OLED 任务；单独测试屏幕时继续使用 `HARDWARE_TEST_OLED`。
