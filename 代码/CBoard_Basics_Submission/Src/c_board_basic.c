#include "c_board_basic.h"

#include <stdio.h>
#include <string.h>

#include "bmi088.h"
#include "dbus_receiver.h"

#define BUZZER_TIMER_CLOCK_HZ 84000000U
#define BUZZER_COUNTER_CLOCK_HZ 1000000U
#define LED_PWM_MAXIMUM 1000U
#define IMU_PRINT_INTERVAL_MS 100U
#define REMOTE_PRINT_INTERVAL_MS 100U

typedef struct
{
  uint16_t frequency_hz;
  uint16_t duration_ms;
} startup_tone_t;

static const startup_tone_t STARTUP_MELODY[] = {
    {523U, 120U}, {659U, 120U}, {784U, 120U}, {1047U, 240U}, {0U, 80U}};

static TIM_HandleTypeDef *buzzer_timer;
static TIM_HandleTypeDef *led_timer;
static UART_HandleTypeDef *dbus_uart;
static UART_HandleTypeDef *host_uart;
static bmi088_t imu;
static bmi088_data_t imu_data;
static dbus_receiver_t remote;
static uint8_t dbus_rx_buffer[DBUS_FRAME_LENGTH];
static bool imu_is_ready;
static uint8_t startup_tone_index;
static uint32_t startup_tone_started_ms;
static uint32_t previous_led_update_ms;
static uint32_t previous_imu_print_ms;
static uint32_t previous_remote_print_ms;

static void buzzer_set_frequency(uint16_t frequency_hz)
{
  if (frequency_hz == 0U)
  {
    (void)HAL_TIM_PWM_Stop(buzzer_timer, TIM_CHANNEL_3);
    return;
  }

  __HAL_TIM_SET_PRESCALER(buzzer_timer, BUZZER_TIMER_CLOCK_HZ / BUZZER_COUNTER_CLOCK_HZ - 1U);
  __HAL_TIM_SET_AUTORELOAD(buzzer_timer, BUZZER_COUNTER_CLOCK_HZ / frequency_hz - 1U);
  __HAL_TIM_SET_COMPARE(buzzer_timer, TIM_CHANNEL_3,
                         (BUZZER_COUNTER_CLOCK_HZ / frequency_hz) / 2U);
  __HAL_TIM_SET_COUNTER(buzzer_timer, 0U);
  (void)HAL_TIM_PWM_Start(buzzer_timer, TIM_CHANNEL_3);
}

static void led_set_rgb(uint16_t red, uint16_t green, uint16_t blue)
{
  __HAL_TIM_SET_COMPARE(led_timer, TIM_CHANNEL_3, red);
  __HAL_TIM_SET_COMPARE(led_timer, TIM_CHANNEL_2, green);
  __HAL_TIM_SET_COMPARE(led_timer, TIM_CHANNEL_1, blue);
}

static void led_update_flowing_pattern(uint32_t now_ms)
{
  if (now_ms - previous_led_update_ms < 80U)
  {
    return;
  }
  previous_led_update_ms = now_ms;

  /* The received D7 frame changes the LED to cyan, proving communication visually. */
  if (dbus_receiver_is_online(&remote, now_ms))
  {
    led_set_rgb(0U, LED_PWM_MAXIMUM, LED_PWM_MAXIMUM);
    return;
  }

  const uint8_t step = (uint8_t)((now_ms / 80U) % 9U);
  if (step < 3U)
  {
    led_set_rgb(LED_PWM_MAXIMUM, 0U, 0U);
  }
  else if (step < 6U)
  {
    led_set_rgb(0U, LED_PWM_MAXIMUM, 0U);
  }
  else
  {
    led_set_rgb(0U, 0U, LED_PWM_MAXIMUM);
  }
}

static void startup_melody_process(uint32_t now_ms)
{
  if (startup_tone_index >= (sizeof(STARTUP_MELODY) / sizeof(STARTUP_MELODY[0])))
  {
    return;
  }
  if (now_ms - startup_tone_started_ms < STARTUP_MELODY[startup_tone_index].duration_ms)
  {
    return;
  }

  ++startup_tone_index;
  startup_tone_started_ms = now_ms;
  if (startup_tone_index < (sizeof(STARTUP_MELODY) / sizeof(STARTUP_MELODY[0])))
  {
    buzzer_set_frequency(STARTUP_MELODY[startup_tone_index].frequency_hz);
  }
}

static void host_print(const char *text)
{
  const uint16_t length = (uint16_t)strlen(text);
  (void)HAL_UART_Transmit(host_uart, (uint8_t *)text, length, 20U);
}

static void telemetry_process(uint32_t now_ms)
{
  char line[192];
  if (imu_is_ready && now_ms - previous_imu_print_ms >= IMU_PRINT_INTERVAL_MS)
  {
    previous_imu_print_ms = now_ms;
    if (bmi088_read(&imu, &imu_data))
    {
      (void)snprintf(line, sizeof(line),
                     "IMU,ACC,mps2,%.3f,%.3f,%.3f,GYRO,rps,%.3f,%.3f,%.3f,T,%.2f\r\n",
                     (double)imu_data.acceleration_mps2[0], (double)imu_data.acceleration_mps2[1],
                     (double)imu_data.acceleration_mps2[2], (double)imu_data.angular_rate_rps[0],
                     (double)imu_data.angular_rate_rps[1], (double)imu_data.angular_rate_rps[2],
                     (double)imu_data.temperature_c);
      host_print(line);
    }
  }

  if (dbus_receiver_is_online(&remote, now_ms) &&
      now_ms - previous_remote_print_ms >= REMOTE_PRINT_INTERVAL_MS)
  {
    previous_remote_print_ms = now_ms;
    (void)snprintf(line, sizeof(line), "RC,%.2f,%.2f,%.2f,%.2f,SW,%u,%u\r\n",
                   (double)remote.left_horizontal, (double)remote.left_vertical,
                   (double)remote.right_horizontal, (double)remote.right_vertical,
                   (unsigned int)remote.left_switch, (unsigned int)remote.right_switch);
    host_print(line);
  }
}

void c_board_basic_init(SPI_HandleTypeDef *imu_spi, TIM_HandleTypeDef *buzzer_timer_handle,
                        TIM_HandleTypeDef *led_timer_handle, UART_HandleTypeDef *dbus_uart_handle,
                        UART_HandleTypeDef *host_uart_handle)
{
  buzzer_timer = buzzer_timer_handle;
  led_timer = led_timer_handle;
  dbus_uart = dbus_uart_handle;
  host_uart = host_uart_handle;
  memset(&remote, 0, sizeof(remote));
  imu = (bmi088_t){.spi = imu_spi,
                   .accelerometer_cs_port = GPIOA,
                   .accelerometer_cs_pin = GPIO_PIN_4,
                   .gyroscope_cs_port = GPIOB,
                   .gyroscope_cs_pin = GPIO_PIN_0};

  (void)HAL_TIM_PWM_Start(led_timer, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Start(led_timer, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Start(led_timer, TIM_CHANNEL_3);
  led_set_rgb(0U, 0U, 0U);

  imu_is_ready = (bmi088_init(&imu) == BMI088_STATUS_OK);
  host_print(imu_is_ready ? "C_BOARD_BASIC: IMU ready\r\n" : "C_BOARD_BASIC: IMU init failed\r\n");

  startup_tone_index = 0U;
  startup_tone_started_ms = HAL_GetTick();
  buzzer_set_frequency(STARTUP_MELODY[startup_tone_index].frequency_hz);

  (void)HAL_UARTEx_ReceiveToIdle_DMA(dbus_uart, dbus_rx_buffer, sizeof(dbus_rx_buffer));
  __HAL_DMA_DISABLE_IT(dbus_uart->hdmarx, DMA_IT_HT);
}

void c_board_basic_process(void)
{
  const uint32_t now_ms = HAL_GetTick();
  startup_melody_process(now_ms);
  led_update_flowing_pattern(now_ms);
  telemetry_process(now_ms);
}

void c_board_basic_uart_rx_event(UART_HandleTypeDef *uart, uint16_t received_length)
{
  if (uart != dbus_uart)
  {
    return;
  }

  if (received_length == DBUS_FRAME_LENGTH)
  {
    (void)dbus_receiver_decode(&remote, dbus_rx_buffer, HAL_GetTick());
  }
  (void)HAL_UARTEx_ReceiveToIdle_DMA(dbus_uart, dbus_rx_buffer, sizeof(dbus_rx_buffer));
  __HAL_DMA_DISABLE_IT(dbus_uart->hdmarx, DMA_IT_HT);
}
