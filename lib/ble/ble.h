// BLE peripheral: advertises, accepts one connection, and serves the GAP and
// GATT services. Built in automatically with BLE=1; the OSAL task table in
// lib/core runs ble_init()/ble_process_event() as their own task.
//
// With BLE=1 app_init() runs once after ble_init(), and app_update() is called
// every 10 ms from an OSAL timer instead of a loop: it must not block, so use
// short work and timestamps (hal_systick()) rather than WaitMs().
#ifndef BLE_H
#define BLE_H

#include "types.h"

typedef enum {
    BLE_IDLE,
    BLE_ADVERTISING,
    BLE_CONNECTED,
} ble_state_t;

// Name shown when scanning (max 20 characters). Call from app_init(); the
// default is "LP715".
void ble_set_name(const char *name);
ble_state_t ble_state(void);

// Used by lib/core.
void ble_init(uint8 task_id);
uint16 ble_process_event(uint8 task_id, uint16 events);

#endif
