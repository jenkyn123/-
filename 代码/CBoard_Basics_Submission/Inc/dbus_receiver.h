#ifndef DBUS_RECEIVER_H
#define DBUS_RECEIVER_H

#include <stdbool.h>
#include <stdint.h>

#define DBUS_FRAME_LENGTH 18U

typedef enum
{
  DBUS_SWITCH_DOWN = 0,
  DBUS_SWITCH_MIDDLE,
  DBUS_SWITCH_UP
} dbus_switch_position_t;

typedef struct
{
  float right_horizontal;
  float right_vertical;
  float left_horizontal;
  float left_vertical;
  float dial;
  dbus_switch_position_t left_switch;
  dbus_switch_position_t right_switch;
  uint16_t keyboard;
  uint32_t last_update_ms;
} dbus_receiver_t;

bool dbus_receiver_decode(dbus_receiver_t *receiver, const uint8_t frame[DBUS_FRAME_LENGTH],
                          uint32_t timestamp_ms);
bool dbus_receiver_is_online(const dbus_receiver_t *receiver, uint32_t now_ms);

#endif  // DBUS_RECEIVER_H
