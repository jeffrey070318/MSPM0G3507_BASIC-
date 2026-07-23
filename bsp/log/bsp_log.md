# bsp_log

<p align='right'>neozng1@hnu.edu.cn</p>

## 使用说明

bsp_log是基于segger RTT实现的日志打印模块。

推荐使用`bsp_log.h`中提供了三级日志：

```c
#define LOGINFO(format,...)
#define LOGWARNING(format,...)
#define LOGERROR(format,...)
```

分别用于输出不同等级的日志。注意RTT不支持直接使用`%f`进行浮点格式化,要使用`void Float2Str(char *str, float va);`转化成字符串之后再发送。

RTT需要调试器能够持续读取目标RAM。J-Link RTT、Ozone或支持RTT内存读取的调试后端均可使用。

另外，若你使用的是cmsis-dap和daplink，**请在 *jlink* 调试任务启动之后再打开`log`任务。**（均在项目文件夹下的.vsocde/task.json中，有注释自行查看）。否则可能出线RTT viewer无法连接客户端的情况。

在ozone中查看log输出，直接打开console调试任务台和terminal调试中断便可看到调试输出。

> 由于ozone版本的原因，可能出现日志不换行或没有颜色。

## 自定义输出

你也可以自定义输出格式，详见Segger RTT的文档。

```c
int PrintLog(const char *fmt, ...);
void Float2Str(char *str, float va); // 输出浮点需要先用此函数进行转换
int Float2StrEx(char *str, size_t size, float va); // 推荐使用，带长度检查
```

调用第一个函数，可以通过jlink或dap-link向调试器连接的上位机发送信息，格式和printf相同，示例如下：

```c
PrintLog("Hello World!\n");
PrintLog("Motor %d met some problem, error code %d!\n",3,1);
```

第二个函数可以将浮点类型转换成字符串以方便发送：

```c
float current_feedback=114.514;
char str_buff[FLOAT2STR_BUFFER_SIZE];
Float2StrEx(str_buff, sizeof(str_buff), current_feedback);
PrintLog("current=%s\n", str_buff);
```
