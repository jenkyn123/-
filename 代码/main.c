/*
 * Replace the USER CODE sections in the CubeMX-generated main.c with this file's
 * user code. Do not replace the generated clock and peripheral initialization.
 */
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

#include "c_board_basic.h"

void SystemClock_Config(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();

  c_board_basic_init(&hspi1, &htim4, &htim5, &huart3, &huart6);

  while (1)
  {
    c_board_basic_process();
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  c_board_basic_uart_rx_event(huart, size);
}
