// Charge monitor: screen and backlight off, charging run by lib/power's
// charge_poll() (fast charge, stopped at CHARGE_FULL_MV), and the resting
// battery voltage logged after every measurement, about every 10 s.
#include "app.h"
#include "display/jd9850.h"
#include "power/power.h"

static const char *const names[] = {"no charger", "charging", "full"};
static int logged_mv = -2;

void app_init(void) {
    lcd_init();
    lcd_off();
    power_init();
    LOG("charge: screen off");
}

void app_update(void) {
    charge_poll();
    int mv = charge_rest_mv();
    if (mv != logged_mv) {
        LOG("battery %d mV %d%%, %s", mv, mv < 0 ? 0 : battery_percent(mv), names[charge_state()]);
        logged_mv = mv;
    }
    WaitMs(10);
}
