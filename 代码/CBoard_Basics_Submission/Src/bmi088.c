#include "bmi088.h"

#define BMI088_ACCELEROMETER_ID 0x1EU
#define BMI088_GYROSCOPE_ID 0x0FU

#define BMI088_ACCELEROMETER_CHIP_ID_REGISTER 0x00U
#define BMI088_ACCELEROMETER_DATA_REGISTER 0x12U
#define BMI088_ACCELEROMETER_TEMPERATURE_REGISTER 0x22U
#define BMI088_ACCELEROMETER_CONFIGURATION_REGISTER 0x40U
#define BMI088_ACCELEROMETER_RANGE_REGISTER 0x41U
#define BMI088_ACCELEROMETER_POWER_CONFIGURATION_REGISTER 0x7CU
#define BMI088_ACCELEROMETER_POWER_CONTROL_REGISTER 0x7DU
#define BMI088_ACCELEROMETER_RESET_REGISTER 0x7EU

#define BMI088_GYROSCOPE_CHIP_ID_REGISTER 0x00U
#define BMI088_GYROSCOPE_DATA_REGISTER 0x02U
#define BMI088_GYROSCOPE_RANGE_REGISTER 0x0FU
#define BMI088_GYROSCOPE_BANDWIDTH_REGISTER 0x10U
#define BMI088_GYROSCOPE_POWER_MODE_REGISTER 0x11U
#define BMI088_GYROSCOPE_RESET_REGISTER 0x14U

#define BMI088_SOFT_RESET_VALUE 0xB6U
#define BMI088_SPI_TIMEOUT_MS 10U
#define BMI088_ACCELERATION_SCALE_MPS2 (9.80665F * 6.0F / 32768.0F)
#define BMI088_ANGULAR_RATE_SCALE_RPS (2000.0F * 3.14159265358979323846F / (180.0F * 32767.0F))

static bool bmi088_accelerometer_read(bmi088_t *bmi088, uint8_t register_address, uint8_t *data,
                                      uint16_t length)
{
  uint8_t transmit_buffer[8] = {0};
  uint8_t receive_buffer[8] = {0};
  if (length > 6U)
  {
    return false;
  }

  transmit_buffer[0] = register_address | 0x80U;
  HAL_GPIO_WritePin(bmi088->accelerometer_cs_port, bmi088->accelerometer_cs_pin, GPIO_PIN_RESET);
  const HAL_StatusTypeDef status =
      HAL_SPI_TransmitReceive(bmi088->spi, transmit_buffer, receive_buffer, length + 2U,
                              BMI088_SPI_TIMEOUT_MS);
  HAL_GPIO_WritePin(bmi088->accelerometer_cs_port, bmi088->accelerometer_cs_pin, GPIO_PIN_SET);
  if (status != HAL_OK)
  {
    return false;
  }

  for (uint16_t index = 0; index < length; ++index)
  {
    data[index] = receive_buffer[index + 2U];
  }
  return true;
}

static bool bmi088_gyroscope_read(bmi088_t *bmi088, uint8_t register_address, uint8_t *data,
                                  uint16_t length)
{
  uint8_t transmit_buffer[7] = {0};
  uint8_t receive_buffer[7] = {0};
  if (length > 6U)
  {
    return false;
  }

  transmit_buffer[0] = register_address | 0x80U;
  HAL_GPIO_WritePin(bmi088->gyroscope_cs_port, bmi088->gyroscope_cs_pin, GPIO_PIN_RESET);
  const HAL_StatusTypeDef status =
      HAL_SPI_TransmitReceive(bmi088->spi, transmit_buffer, receive_buffer, length + 1U,
                              BMI088_SPI_TIMEOUT_MS);
  HAL_GPIO_WritePin(bmi088->gyroscope_cs_port, bmi088->gyroscope_cs_pin, GPIO_PIN_SET);
  if (status != HAL_OK)
  {
    return false;
  }

  for (uint16_t index = 0; index < length; ++index)
  {
    data[index] = receive_buffer[index + 1U];
  }
  return true;
}

static bool bmi088_write(bmi088_t *bmi088, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                         uint8_t register_address, uint8_t value)
{
  uint8_t transmit_buffer[2] = {register_address, value};
  HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
  const HAL_StatusTypeDef status =
      HAL_SPI_Transmit(bmi088->spi, transmit_buffer, sizeof(transmit_buffer), BMI088_SPI_TIMEOUT_MS);
  HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
  return status == HAL_OK;
}

static bool bmi088_write_and_check(bmi088_t *bmi088, bool accelerometer, uint8_t register_address,
                                   uint8_t value)
{
  uint8_t read_value = 0U;
  GPIO_TypeDef *cs_port = accelerometer ? bmi088->accelerometer_cs_port : bmi088->gyroscope_cs_port;
  const uint16_t cs_pin = accelerometer ? bmi088->accelerometer_cs_pin : bmi088->gyroscope_cs_pin;
  if (!bmi088_write(bmi088, cs_port, cs_pin, register_address, value))
  {
    return false;
  }
  HAL_Delay(1U);
  return accelerometer
             ? (bmi088_accelerometer_read(bmi088, register_address, &read_value, 1U) &&
                read_value == value)
             : (bmi088_gyroscope_read(bmi088, register_address, &read_value, 1U) &&
                read_value == value);
}

bmi088_status_t bmi088_init(bmi088_t *bmi088)
{
  uint8_t chip_id = 0U;
  /* An initial SPI read generates the CS rising edge that selects the accelerometer SPI mode. */
  (void)bmi088_accelerometer_read(bmi088, BMI088_ACCELEROMETER_CHIP_ID_REGISTER, &chip_id, 1U);
  HAL_Delay(1U);
  if (!bmi088_write(bmi088, bmi088->accelerometer_cs_port, bmi088->accelerometer_cs_pin,
                    BMI088_ACCELEROMETER_RESET_REGISTER, BMI088_SOFT_RESET_VALUE))
  {
    return BMI088_STATUS_ACCEL_NOT_FOUND;
  }
  HAL_Delay(50U);
  /* Reset restores the accelerometer's I2C default; this dummy transaction switches it back to SPI. */
  (void)bmi088_accelerometer_read(bmi088, BMI088_ACCELEROMETER_CHIP_ID_REGISTER, &chip_id, 1U);
  HAL_Delay(1U);
  if (!bmi088_accelerometer_read(bmi088, BMI088_ACCELEROMETER_CHIP_ID_REGISTER, &chip_id, 1U) ||
      chip_id != BMI088_ACCELEROMETER_ID)
  {
    return BMI088_STATUS_ACCEL_NOT_FOUND;
  }

  if (!bmi088_write_and_check(bmi088, true, BMI088_ACCELEROMETER_POWER_CONTROL_REGISTER, 0x04U) ||
      !bmi088_write_and_check(bmi088, true, BMI088_ACCELEROMETER_POWER_CONFIGURATION_REGISTER, 0x00U) ||
      !bmi088_write_and_check(bmi088, true, BMI088_ACCELEROMETER_CONFIGURATION_REGISTER, 0xABU) ||
      !bmi088_write_and_check(bmi088, true, BMI088_ACCELEROMETER_RANGE_REGISTER, 0x01U))
  {
    return BMI088_STATUS_CONFIGURATION_FAILED;
  }

  if (!bmi088_write(bmi088, bmi088->gyroscope_cs_port, bmi088->gyroscope_cs_pin,
                    BMI088_GYROSCOPE_RESET_REGISTER, BMI088_SOFT_RESET_VALUE))
  {
    return BMI088_STATUS_GYRO_NOT_FOUND;
  }
  HAL_Delay(50U);
  if (!bmi088_gyroscope_read(bmi088, BMI088_GYROSCOPE_CHIP_ID_REGISTER, &chip_id, 1U) ||
      chip_id != BMI088_GYROSCOPE_ID)
  {
    return BMI088_STATUS_GYRO_NOT_FOUND;
  }

  if (!bmi088_write_and_check(bmi088, false, BMI088_GYROSCOPE_RANGE_REGISTER, 0x00U) ||
      !bmi088_write_and_check(bmi088, false, BMI088_GYROSCOPE_BANDWIDTH_REGISTER, 0x81U) ||
      !bmi088_write_and_check(bmi088, false, BMI088_GYROSCOPE_POWER_MODE_REGISTER, 0x00U))
  {
    return BMI088_STATUS_CONFIGURATION_FAILED;
  }
  return BMI088_STATUS_OK;
}

bool bmi088_read(bmi088_t *bmi088, bmi088_data_t *data)
{
  uint8_t acceleration_bytes[6] = {0};
  uint8_t angular_rate_bytes[6] = {0};
  uint8_t temperature_bytes[2] = {0};
  if (!bmi088_accelerometer_read(bmi088, BMI088_ACCELEROMETER_DATA_REGISTER, acceleration_bytes,
                                 sizeof(acceleration_bytes)) ||
      !bmi088_gyroscope_read(bmi088, BMI088_GYROSCOPE_DATA_REGISTER, angular_rate_bytes,
                             sizeof(angular_rate_bytes)) ||
      !bmi088_accelerometer_read(bmi088, BMI088_ACCELEROMETER_TEMPERATURE_REGISTER,
                                 temperature_bytes, sizeof(temperature_bytes)))
  {
    return false;
  }

  for (uint8_t axis = 0U; axis < 3U; ++axis)
  {
    const int16_t acceleration_raw =
        (int16_t)((acceleration_bytes[axis * 2U + 1U] << 8) | acceleration_bytes[axis * 2U]);
    const int16_t angular_rate_raw =
        (int16_t)((angular_rate_bytes[axis * 2U + 1U] << 8) | angular_rate_bytes[axis * 2U]);
    data->acceleration_mps2[axis] = (float)acceleration_raw * BMI088_ACCELERATION_SCALE_MPS2;
    data->angular_rate_rps[axis] = (float)angular_rate_raw * BMI088_ANGULAR_RATE_SCALE_RPS;
  }

  int16_t temperature_raw =
      (int16_t)((temperature_bytes[0] << 3) | (temperature_bytes[1] >> 5));
  if (temperature_raw > 1023)
  {
    temperature_raw -= 2048;
  }
  data->temperature_c = (float)temperature_raw * 0.125F + 23.0F;
  return true;
}
