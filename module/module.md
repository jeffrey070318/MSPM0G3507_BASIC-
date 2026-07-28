# Module 层

Module 层组合 BSP 能力，向 app 提供可复用设备和算法，不直接拥有任务调度。

当前主要模块：

- `algorithm`：平台无关的 PID 等算法；
- `buzzer`：低电平有效的有源蜂鸣器，初始化和关闭保持高电平；
- `led`：调用者持有的多实例 LED 开、关和翻转控制；
- `motor`：支持 DRV8701E/TB6612 的开环与编码器速度闭环电机；
- `gray_sensor`：三位地址选择、单路读取的八路灰度传感器；
- `imu`：JY901S I2C 数据读取与换算；
- `key`：调用者持有的多实例按键电平读取；
- `message_center`：发布/订阅消息中心；
- `oled`：OLED 显示；
- `servo`：调用者持有的多实例舵机角度控制；
- `vofa`：VOFA+ JustFloat 输出。

Module 对象默认由 app 静态持有。底层 BSP 资源只在启动期注册一次，并一直持有到 MCU 复位。
