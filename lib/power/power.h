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
bool charger_present(void);

#endif
