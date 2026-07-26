# MSPM0G3507_BASIC

面向 MSPM0G3507 的控制类项目基础工程，采用 `app/module/bsp` 三层结构，使用 SysConfig、CMake、Arm GNU Toolchain 和 FreeRTOS。工程也保留 NoRTOS 构建，用于底层移植和最小验证。

## 快速入口

- [框架使用说明](./框架使用说明.md)：环境、构建、烧录、硬件测试和当前适配状态。
- [引脚使用说明](./引脚使用说明.md)：SysConfig 引脚映射及外部电气要求。
- [APP 层指引](./app/APP层应用编写指引.md)：任务和应用代码的组织方式。
- [Module 层](./module/module.md)：可复用设备与算法模块索引。
- [宿主测试](./tests/README.md)：不连接硬件即可运行的逻辑测试。

## 构建

```powershell
cmake -S . -B build -G Ninja -DUSE_FREERTOS=ON
cmake --build build --parallel

cmake -S . -B build-nortos -G Ninja -DUSE_FREERTOS=OFF
cmake --build build-nortos --parallel
```

引脚、时钟和外设实例只修改根目录的 `MSPM0G3507_BASIC.syscfg`。`build/syscfg` 下的文件由 SysConfig 自动生成，不应手工编辑。

## 烧录

工程提供 CMSIS-DAP/DAPLink 和 XDS110 两套 OpenOCD 入口：

```powershell
cmake --build build --target flash
cmake --build build --target flash-xds110
```

XDS110 已完成实机烧录验证。当前仍存在部分烧录操作约 51 秒后才完成的问题，功能可用，但延迟原因尚未定位。

## 当前硬件状态

- OLED I2C 显示已验证。
- 电机 1、左编码器和 DRV8701E 速度闭环已验证。
- XDS110 烧录已验证。
- UART2/3 DMA、超时接收和环形缓冲已完成代码与构建验证，仍需连续数据流硬件压力测试。
- 右电机方向、右编码器方向、整车运动学、灰度模块电平及 Flash 参数区仍需实测。
