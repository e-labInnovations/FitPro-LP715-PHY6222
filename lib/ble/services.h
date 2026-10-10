// GATT services on top of lib/ble: the standard Battery Service and an LP715
// service for controlling the watch and hearing from it.
//
// Battery Service (0x180F): Battery Level (0x2A19), read and notify, 0-100.
//
// LP715 service 4c500001-3731-3500-b000-000000000000:
//   Control 4c500002-…  write / write without response. The first byte is
//                       the command, the rest its argument:
//                         01 lo hi  buzz for lo + hi*256 ms (max 2000)
//                         02 b      heart-rate LED off (0) / on (1)
//                         03 n      backlight level 0 (off) .. 4 (full)
//                         04        reboot into the OTA bootloader (OTA=1
//                                   builds; see lib/core/ota.h)
//   Events  4c500003-…  notify, one byte per event (svc_event_t).
#ifndef BLE_SERVICES_H
#define BLE_SERVICES_H

#include "types.h"

typedef enum {
    SVC_CMD_BUZZ = 0x01,
    SVC_CMD_LED = 0x02,
    SVC_CMD_BACKLIGHT = 0x03,
    SVC_CMD_OTA = 0x04,
} svc_cmd_t;

typedef enum {
    SVC_EVENT_TAP = 0x01,
    SVC_EVENT_LONG = 0x02,
    SVC_EVENT_VERY_LONG = 0x03,
    SVC_EVENT_SHAKE = 0x10,
} svc_event_t;

// Called with each write to Control; data[0] is the command. Runs in the BLE
// stack's task, so keep it short.
typedef void (*svc_command_cb_t)(const uint8 *data, uint8 len);

// Registers both services. Call from app_init().
void svc_init(svc_command_cb_t on_command);

// Sets the battery level and notifies a subscribed client if it changed.
void svc_battery_set(uint8 percent);

// Notifies a subscribed client of an event; does nothing otherwise.
void svc_event(svc_event_t event);

#endif
