# C Board 基本功能（STM32F407IGHx）

这是一个集成工程，不是四个互相冲突的 `main()` 工程。STM32CubeMX 生成 HAL/启动/CMake 工程框架；`Src/c_board_basic.c` 等应用代码把四项验收功能接到 CubeMX 初始化出的外设上。CubeMX 工程配置文件为 `CBoard_Basics_Complete.ioc`。

## 已包含的四项功能代码

1. 上电提示音：TIM4_CH3（PD14）输出一段启动旋律。
2. RGB 流水灯：TIM5_CH1/2/3（PH10/PH11/PH12）输出 PWM，循环显示红、绿、蓝；检测到遥控器帧时显示青色。
3. IMU 串口打印：BMI088 由 SPI1（PB3=SCK、PB4=MISO、PA7=MOSI）读取，CS 使用 PA4/PB0；数据经 USART6_TX（PG14，115200）输出。
4. 遥控器通信：USART3（PC10/PC11，100000、偶校验、9 位字长）接收 DBUS；遥控数据解析后也从 USART6 输出。

## 用 VSCode 打开和构建

在 VSCode 中打开本文件夹 `CBoard_Basics_Submission`。构建需要本机已安装并能从终端找到：CMake 3.22 或更高版本、Ninja、GNU Arm Embedded Toolchain（`arm-none-eabi-gcc`）。`.vscode/extensions.json` 提供推荐扩展。

工具链安装完成后，按 `Ctrl+Shift+B` 运行默认任务 `C Board: Build Debug`。该任务会先执行 Debug 配置，再编译整个 CMake 工程。也可从终端在本目录执行 `cmake --preset Debug`，然后 `cmake --build --preset Debug`。成功构建后，ELF 位于 `build/Debug/CBoard_Basics_Complete.elf`。

`.vscode` 只保存适用于本目录的编辑器和扩展推荐设置。旧 VSCode 模板中的 OpenOCD 烧录/调试任务依赖项目外的 `openocd.cfg` 和未确认的调试器配置，因此没有照搬，避免提交一个失效的烧录按钮。

## 重要验证状态

- `.ioc`、CubeMX 生成的 HAL/CMake/启动文件以及四项功能源文件均已放在本目录。
- 当前环境没有检测到 CMake、Ninja、`arm-none-eabi-gcc`、OpenOCD 或 STM32CubeProgrammer 命令，因此尚未执行本机 CMake 配置/编译，也没有生成可烧录的 ELF/HEX/BIN。上面的 VSCode 构建快捷任务已放入工程，但在这些工具安装并加入 PATH 前不能成功构建。
- 要实际烧录，还需先构建成功，并安装/配置 ST-LINK 驱动和 STM32CubeProgrammer（或自行配置匹配的 OpenOCD），连接目标板与 ST-LINK。烧录操作由使用者执行。
- 代码已按工程文件和外设初始化接口集成，但尚未在实物 C 板上验证 BMI088 接线/CS、遥控器接收机串口极性与帧流、板载 LED/蜂鸣器引脚等硬件行为。请先对照你手中的 C 板原理图核对这些引脚再上电测试。

## CubeMX 配置注意

打开 `CBoard_Basics_Complete.ioc` 可查看芯片/引脚和外设配置。工程来自 CubeMX 生成的 HAL/CMake 框架并在生成代码的用户区接入应用逻辑；当前 `.ioc` 还保留课程模板中与四项功能无关的外设/中间件条目。重新 Generate Code 前，请先在 CubeMX 中确认不启用 FreeRTOS 调度入口并保留 `USER CODE` 区域；重新生成后应再次检查 `Src/main.c` 和构建配置，避免模板项覆盖应用入口或误把无关模块引入构建。
