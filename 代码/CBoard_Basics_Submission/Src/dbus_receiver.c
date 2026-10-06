#include "dbus_receiver.h"

#include <math.h>

static float dbus_normalize_channel(uint16_t raw_value)
{
  return ((float)raw_value - 1024.0F) / 660.0F;
}

static dbus_switch_position_t dbus_decode_switch(uint8_t raw_value)
{
  if (raw_value == 1U)
  {
    return DBUS_SWITCH_UP;
  }
  if (raw_value == 3U)
  {
    return DBUS_SWITCH_MIDDLE;
  }
  return DBUS_SWITCH_DOWN;
}

bool dbus_receiver_decode(dbus_receiver_t *receiver, const uint8_t frame[DBUS_FRAME_LENGTH],
                          uint32_t timestamp_ms)
{
  const uint16_t right_horizontal = (uint16_t)((frame[0] | (frame[1] << 8)) & 0x07FFU);
  const uint16_t right_vertical = (uint16_t)(((frame[1] >> 3) | (frame[2] << 5)) & 0x07FFU);
  const uint16_t left_horizontal =
      (uint16_t)(((frame[2] >> 6) | (frame[3] << 2) | (frame[4] << 10)) & 0x07FFU);
  const uint16_t left_vertical = (uint16_t)(((frame[4] >> 1) | (frame[5] << 7)) & 0x07FFU);
  const uint16_t dial = (uint16_t)((frame[16] | (frame[17] << 8)) & 0x07FFU);

  const float normalized_right_horizontal = dbus_normalize_channel(right_horizontal);
  const float normalized_right_vertical = dbus_normalize_channel(right_vertical);
  const float normalized_left_horizontal = dbus_normalize_channel(left_horizontal);
  const float normalized_left_vertical = dbus_normalize_channel(left_vertical);

  /* A channel outside this range usually means a misaligned 18-byte frame. */
  if (fabsf(normalized_right_horizontal) > 1.2F || fabsf(normalized_right_vertical) > 1.2F ||
      fabsf(normalized_left_horizontal) > 1.2F || fabsf(normalized_left_vertical) > 1.2F)
  {
    return false;
  }

  receiver->right_horizontal = normalized_right_horizontal;
  receiver->right_vertical = normalized_right_vertical;
  receiver->left_horizontal = normalized_left_horizontal;
  receiver->left_vertical = normalized_left_vertical;
  receiver->dial = dbus_normalize_channel(dial);
  receiver->right_switch = dbus_decode_switch((frame[5] >> 4) & 0x03U);
  receiver->left_switch = dbus_decode_switch((frame[5] >> 6) & 0x03U);
  receiver->keyboard = (uint16_t)(frame[14] | (frame[15] << 8));
  receiver->last_update_ms = timestamp_ms;
  return true;
}

bool dbus_receiver_is_online(const dbus_receiver_t *receiver, uint32_t now_ms)
{
  return receiver->last_update_ms != 0U && (now_ms - receiver->last_update_ms) <= 100U;
}
