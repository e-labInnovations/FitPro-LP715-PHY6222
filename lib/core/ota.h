// Hand over to the stock OTA bootloader (the SDK's OTA_internal_flash), for
// apps built with OTA=1. The bootloader reads the low nibble of the AON
// register 0x4000f034 at boot: 0 runs the app, 2 stays in OTA mode and
// advertises the SDK's OTA service (5833ff01-9b8b-5191-6142-22a4536ef123).
// It clears the register itself, so the next reset runs the app again.
#ifndef OTA_H
#define OTA_H

#include "clock.h"

#define OTA_MODE_SELECT_REG (*(volatile uint32_t *)0x4000f034)
#define OTA_MODE_OTA 2

static inline void ota_reboot(void) {
    OTA_MODE_SELECT_REG = OTA_MODE_OTA;
    hal_system_soft_reset();
}

#endif
