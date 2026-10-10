// Charge monitor: screen and backlight off so the charger's current goes into
// the cell, and the battery voltage and charger state logged every 5 s. Useful
// for reviving a drained cell, whose charger current otherwise barely covers
// the watch's own draw.
#include "app.h"
#include "display/jd9850.h"
#include "power/power.h"

void app_init(void) {
    lcd_init();
    lcd_off();
    power_init();
    LOG("charge: screen off, logging every 5 s");
}

void app_update(void) {
    int mv = battery_mv();
    LOG("battery %d mV %d%%, charger %s", mv, mv < 0 ? 0 : battery_percent(mv),
        charger_present() ? "yes" : "no");
    WaitMs(5000);
}
