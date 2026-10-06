# C Board basic functions

This C/HAL application covers the four requested checks:

1. The buzzer plays a non-blocking four-note startup melody.
2. The RGB LED runs red-green-blue flowing lights. It becomes cyan after valid D7 data arrives.
3. The host UART emits BMI088 accelerometer and gyroscope three-axis values every 100 ms.
4. USART3 receives and validates 18-byte D7 DBUS frames, then reports joystick and switch data.

## Generated CubeMX configuration

Generate an STM32F407 project with the HAL driver. These settings are required; the code deliberately
does not reconfigure CubeMX-owned files.

| Peripheral | C board connection | Required configuration |
| --- | --- | --- |
| SPI1 | PA4: ACC CS, PB0: GYRO CS, PB3/PB4/PA7 | Master, CPOL high, second edge, software NSS, prescaler 8 or slower |
| TIM4 CH3 | PD14 buzzer | PWM, timer input clock 84 MHz |
| TIM5 CH1/2/3 | PH10 blue / PH11 green / PH12 red | PWM, period 999; all three channels enabled |
| USART3 RX + DMA | PC11 DBUS | 100000 baud, 9-bit word length, even parity, one stop bit, Receive-to-Idle DMA |
| USART6 TX | PG14 host serial | 115200 baud, 8N1 |
| GPIO outputs | PA4, PB0 | Push-pull, initial high (BMI088 chip-select inactive) |

Add `Core/Inc` to include paths and add all `Core/Src/*.c` files to the project. `main.c` assumes the
normal CubeMX-generated `SystemClock_Config()` declaration and `MX_DMA_Init()` are present.
If the toolchain uses newlib-nano, add `-u _printf_float` to the linker flags; otherwise the telemetry
fields printed with `%.3f` will be blank or incorrect.

## Verification

Open a serial terminal at 115200 baud on UART6. Expected output includes:

```text
C_BOARD_BASIC: IMU ready
IMU,ACC,mps2,0.012,-0.034,9.781,GYRO,rps,0.002,-0.001,0.000,T,31.25
RC,-0.02,0.01,0.00,-0.01,SW,1,1
```

The exact axis directions depend on board installation, so this code reports sensor-frame values rather
than silently applying an unverified robot-coordinate transform.
