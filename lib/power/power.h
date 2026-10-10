// Battery voltage and charger detection. Needs the SDK ADC driver:
//   SDK_EXTRA = components/driver/adc/adc.c
#ifndef POWER_H
#define POWER_H

#include "types.h"

void power_init(void);
// Battery voltage in mV (P14 through the 1/5.5 divider); -1 if the ADC failed.
int battery_mv(void);
// Rough state of charge, 0-100, from a typical Li-ion discharge curve.
int battery_percent(int mv);
// Valid only while CHG_CTL is LOW; charge_poll() keeps track while charging.
bool charger_present(void);

// Charging, as the stock firmware does it: with a charger attached CHG_CTL is
// HIGH (fast charge); every 10 s it goes LOW for 300 ms to measure the resting
// battery voltage and check the charger, and stays LOW once the cell is full
// (CHARGE_FULL_MV). Call from app_update(); it never blocks for long.
#define CHARGE_FULL_MV 4088

typedef enum {
    CHARGE_NONE,       // no charger
    CHARGE_ON,         // charging
    CHARGE_FULL,       // charger attached, cell full, charging stopped
} charge_state_t;

void charge_poll(void);
charge_state_t charge_state(void);
// Battery voltage from the last measurement, taken with charging paused; -1
// before the first one. Use it instead of battery_mv() while charging.
int charge_rest_mv(void);

#endif
