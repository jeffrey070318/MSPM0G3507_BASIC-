# VS Code 本机配置

仓库只跟踪可移植任务和配置示例。以下文件包含个人工具链路径，已由 `.gitignore` 排除：

- `.vscode/launch.json`
- `.vscode/settings.json`
- `.vscode/c_cpp_properties.json`
- `CMakeUserPresets.json`

首次克隆后，在仓库根目录执行：

```powershell
Copy-Item .vscode/launch.example.json .vscode/launch.json
Copy-Item .vscode/settings.example.json .vscode/settings.json
Copy-Item .vscode/c_cpp_properties.example.json .vscode/c_cpp_properties.json
Copy-Item CMakeUserPresets.example.json CMakeUserPresets.json
```

然后只修改这些本机文件中的 SDK、SysConfig、CMake、Ninja、OpenOCD 和 Arm GNU Toolchain 路径。不要把个人绝对路径重新加入 Git。

`.vscode/tasks.json` 通过系统 `PATH` 调用 `cmake`，并使用 `local` / `local-no-rtos` 用户预设。VS Code 的 CMSIS-DAP 与 XDS110 调试入口由本机 `launch.json` 提供。
