#ifndef C_BOARD_BASIC_H
#define C_BOARD_BASIC_H

#include "main.h"

/*
 * C Board basic-function application.
 *
 * Required CubeMX peripheral handles:
 *   hspi1   - BMI088 SPI
 *   htim4   - buzzer, TIM4 channel 3 (PD14)
 *   htim5   - RGB LED, channels 1/2/3 (PH10/PH11/PH12)
 *   huart3  - D7 DBUS receiver (PC11 RX, 100000 baud, 8E1)
 *   huart6  - host serial output (PG14 TX, 115200 baud)
 */
void c_board_basic_init(SPI_HandleTypeDef *imu_spi, TIM_HandleTypeDef *buzzer_timer,
                        TIM_HandleTypeDef *led_timer, UART_HandleTypeDef *dbus_uart,
                        UART_HandleTypeDef *host_uart);

/* Call continuously from the main loop.  It contains no blocking delays. */
void c_board_basic_process(void);

/* Forward the HAL Receive-to-Idle callback from main.c. */
void c_board_basic_uart_rx_event(UART_HandleTypeDef *uart, uint16_t received_length);

#endif  // C_BOARD_BASIC_H
