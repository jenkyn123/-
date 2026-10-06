#ifndef BMI088_H
#define BMI088_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

typedef enum
{
  BMI088_STATUS_OK = 0,
  BMI088_STATUS_ACCEL_NOT_FOUND,
  BMI088_STATUS_GYRO_NOT_FOUND,
  BMI088_STATUS_CONFIGURATION_FAILED
} bmi088_status_t;

typedef struct
{
  float acceleration_mps2[3];
  float angular_rate_rps[3];
  float temperature_c;
} bmi088_data_t;

typedef struct
{
  SPI_HandleTypeDef *spi;
  GPIO_TypeDef *accelerometer_cs_port;
  uint16_t accelerometer_cs_pin;
  GPIO_TypeDef *gyroscope_cs_port;
  uint16_t gyroscope_cs_pin;
} bmi088_t;

bmi088_status_t bmi088_init(bmi088_t *bmi088);
bool bmi088_read(bmi088_t *bmi088, bmi088_data_t *data);

#endif  // BMI088_H
