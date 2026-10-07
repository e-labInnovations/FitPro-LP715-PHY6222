// Start OSAL. Adapted from amir1387aht/phy6222_smartwatch (MIT).
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#ifdef __GNUC__
void app_osal_init(void) __attribute__((naked));
#endif
void app_osal_init(void) {
    osal_init_system();
    osal_pwrmgr_device(PWRMGR_BATTERY);
    osal_start_system();
}
