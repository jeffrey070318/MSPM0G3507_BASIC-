# I2C 最小硬件测试设计

## 目标

在不依赖 OLED 初始化流程的情况下，验证 MSPM0G3507 的 I2C0、PA28/PA31 接线以及目标设备应答是否正常。

## 测试方法

- 新增独立的 `HARDWARE_TEST_IIC` 模式，继续由 `hardware_test_config.h` 中的 C 宏选择。
- 使用现有 SysConfig 生成的 I2C0 配置：PA28 为 SDA、PA31 为 SCL、总线速率 100 kHz。
- 将 OLED 的 8 位地址 `0x78`规范化为 DriverLib 使用的 7 位地址 `0x3C`。
- 通过现有 IIC BSP 发送 SSD1306 控制字节 `0x00`和指令 `0xAE`。
- 不修改 `.syscfg`、生成文件和 OLED 模块。

## 可观测结果

Live Watch 暴露注册结果、发送返回值、控制器状态、实际地址、运行次数、成功次数和失败次数。

## 判定

- 返回 `DEVICE_OK`：I2C 控制器、引脚和目标地址的基本发送链路成立。
- 返回错误或超时：记录控制器状态，下一轮再决定进行地址扫描、总线波形检查或 BSP 状态修正。

