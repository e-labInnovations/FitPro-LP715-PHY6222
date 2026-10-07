// Every example provides these two. app_init() runs once after the system and
// BLE controller are up; app_update() is then called in a loop forever.
#ifndef APP_H
#define APP_H

#include "types.h"
#include "gpio.h"
#include "clock.h"
#include "log.h"
#include "board.h"

void app_init(void);
void app_update(void);

#endif
