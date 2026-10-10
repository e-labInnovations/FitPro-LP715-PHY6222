// BLE remote control: the Battery Service and the LP715 service from
// lib/ble/services.h. A client can buzz the motor, switch the heart-rate LED
// and set the backlight, and is notified of taps, long presses and shakes.
// The screen shows the connection, the battery, the last command and the last
// event.
#include "app.h"
#include "ble/ble.h"
#include "ble/services.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "fonts/FreeMono9pt7b.h"
#include "power/power.h"
#include "input/button.h"
#include "vibrate/vibrate.h"
#include "led/led.h"
#include "motion/motion.h"

#define WHITE  RGB565(255, 255, 255)
#define BLACK  RGB565(0, 0, 0)
#define GREY   RGB565(90, 90, 90)
#define BLUE   RGB565(40, 120, 255)
#define GREEN  RGB565(0, 220, 80)
#define YELLOW RGB565(255, 200, 0)
#define CYAN   RGB565(0, 200, 230)

#define BATTERY_TICKS (10 * 1600)   // 10 s in 625 us ticks
#define MAX_BUZZ_MS   2000

// Rows, as text baselines.
#define ROW_STATE   44
#define ROW_BATTERY 68
#define ROW_CMD     104
#define ROW_EVENT   140

// Written by the BLE task in on_command(), handled in app_update().
static volatile bool cmd_pending;
static uint8 cmd[3];

static int shown_state = -1;
static uint32_t last_battery;

static void show(int baseline, uint16_t color, const char *text, int value, bool has_value) {
    gfx_fill_rect(0, baseline - 14, LCD_WIDTH, 20, BLACK);
    gfx_set_text_color(color);
    gfx_set_cursor_no_height(2, baseline);
    gfx_print(text);
    if (has_value)
        gfx_print_int(value);
}

static void update_battery(void) {
    int mv = battery_mv();
    int pct = mv < 0 ? 0 : battery_percent(mv);
    svc_battery_set(pct);
    LOG("battery %d mV %d%%", mv, pct);
    show(ROW_BATTERY, WHITE, "bat ", pct, true);
    last_battery = hal_systick();
}

static void on_command(const uint8 *data, uint8 len) {
    if (cmd_pending)
        return;   // still handling the previous one
    cmd[0] = data[0];
    cmd[1] = len > 1 ? data[1] : 0;
    cmd[2] = len > 2 ? data[2] : 0;
    cmd_pending = true;
}

static void run_command(void) {
    switch (cmd[0]) {
    case SVC_CMD_BUZZ: {
        int ms = cmd[1] | cmd[2] << 8;
        if (ms > MAX_BUZZ_MS)
            ms = MAX_BUZZ_MS;
        vibrate_start(ms);
        show(ROW_CMD, YELLOW, "buzz", 0, false);
        LOG("buzz %d ms", ms);
        break;
    }
    case SVC_CMD_LED:
        led_set(cmd[1]);
        show(ROW_CMD, YELLOW, cmd[1] ? "LED on" : "LED off", 0, false);
        break;
    case SVC_CMD_BACKLIGHT:
        lcd_backlight(cmd[1] > 4 ? 4 : cmd[1]);
        show(ROW_CMD, YELLOW, "light ", cmd[1], true);
        break;
    default:
        show(ROW_CMD, GREY, "cmd ? ", cmd[0], true);
        break;
    }
}

static void event(svc_event_t e, const char *name) {
    svc_event(e);
    show(ROW_EVENT, CYAN, name, 0, false);
    LOG("event %s", name);
}

void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    power_init();
    button_init();
    vibrate_init();
    led_init();
    motion_init();

    show(20, WHITE, "LP715", 0, false);
    ble_set_name("LP715");
    svc_init(on_command);
    update_battery();
    show(ROW_CMD, GREY, "no cmd", 0, false);
    show(ROW_EVENT, GREY, "no evt", 0, false);
}

void app_update(void) {
    vibrate_poll();

    ble_state_t s = ble_state();
    if ((int)s != shown_state) {
        shown_state = s;
        if (s == BLE_CONNECTED)
            show(ROW_STATE, GREEN, "conn.", 0, false);
        else if (s == BLE_ADVERTISING)
            show(ROW_STATE, BLUE, "advert.", 0, false);
        else
            show(ROW_STATE, GREY, "idle", 0, false);
    }

    if (cmd_pending) {
        run_command();
        cmd_pending = false;
    }

    switch (button_poll()) {
    case BUTTON_PRESS:
        event(SVC_EVENT_TAP, "tap");
        break;
    case BUTTON_LONG:
        event(SVC_EVENT_LONG, "long");
        break;
    case BUTTON_VERY_LONG:
        event(SVC_EVENT_VERY_LONG, "v.long");
        break;
    default:
        break;
    }
    if (motion_poll())
        event(SVC_EVENT_SHAKE, "shake");

    if (hal_systick() - last_battery >= BATTERY_TICKS)
        update_battery();
}
