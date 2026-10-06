# C Board STM32F407 工程

实际入口源文件是 `CBoard_Basics_Complete/Src/main.c`，应用逻辑在同目录的
`c_board_basic.c`、`bmi088.c` 和 `dbus_receiver.c` 中。CMake 会将这些源文件、
HAL 驱动与启动汇编一起编译，再链接成 ELF 固件。

`代码/main.c` 是移植参考片段，没有加入当前构建目标。

## 在 VS Code 中构建

本机已安装 `D:/download/STM32CubeCLT_1.22.0`。根目录和固件目录下的
`.vscode/settings.json` 已配置其中的 CMake、Ninja、ARM GCC 路径；换电脑时需调整。
这些设置供 VS Code 使用，不修改系统 PATH。

1. 打开本仓库根目录；设置刚更新时执行 `Developer: Reload Window`。
2. 按 Ctrl+Shift+B，默认任务会先配置 `Debug`，再构建整个固件。
3. 构建结果位于根目录的 `build/Debug/CBoard_Basics_Complete.elf`。

也可以单独打开 `CBoard_Basics_Complete`，在 CMake Tools 中选择 `Debug` 预设后
执行 Configure、Build；此时使用该子目录自己的 `build/Debug`。

如果 `main.h` 下提示找不到 `stddef.h`，应检查编译器配置：这个标准头文件由
ARM GCC 提供。代码检查已接入真实编译器及生成的 `compile_commands.json`。

2026-10-06 已实际完成 Debug 配置、编译和链接。尚未在目标板上烧录和验证硬件功能。
