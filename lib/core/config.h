// Settings the SDK's own sources expect from a project "config.h". Pins are in
// board.h.
#ifndef CONFIG_H
#define CONFIG_H

// BLE connection parameters, published in the GAP service by the SDK's
// gapgattserver.c (Peripheral Preferred Connection Parameters) and requested
// by lib/ble after connecting.
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL 24    // 30 ms, in 1.25 ms units
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL 80    // 100 ms
#define DEFAULT_DESIRED_SLAVE_LATENCY     0
#define DEFAULT_DESIRED_CONN_TIMEOUT      500   // 5 s, in 10 ms units

#endif
