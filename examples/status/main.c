// Status screen: battery percent, voltage and a fill bar, charger state, and
// the last button event. The vibrator answers the button: one buzz for a tap,
// two for a long press, three for a very long press.
#include "app.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "fonts/FreeSans12pt7b.h"
#include "fonts/FreeMono9pt7b.h"
#include "power/power.h"
#include "input/button.h"
#include "vibrate/vibrate.h"

#define WHITE  RGB565(255, 255, 255)
#define BLACK  RGB565(0, 0, 0)
#define GREY   RGB565(90, 90, 90)
#define GREEN  RGB565(0, 200, 60)
#define YELLOW RGB565(255, 200, 0)
#define RED    RGB565(230, 30, 30)
#define CYAN   RGB565(0, 200, 230)

#define BAR_X 8
#define BAR_Y 56
#define BAR_W 58
#define BAR_H 22

static uint32_t last_battery;   // hal_systick() of the last battery update
static int last_charger = -1;
static int shown_pct = -1, shown_mv = -100;
static int presses;

// Redraws only the parts that changed, so nothing flickers once a second.
static void draw_battery(void) {
    int mv = battery_mv();
    int pct = mv < 0 ? 0 : battery_percent(mv);
    bool chg = charger_present();

    if (pct != shown_pct) {
        uint16_t c = pct < 20 ? RED : pct < 40 ? YELLOW : GREEN;
        gfx_fill_rect(0, 0, LCD_WIDTH, 28, BLACK);
        gfx_set_font(&FreeSans12pt7b);
        gfx_set_text_color(WHITE);
        gfx_set_cursor_no_height(8, 24);
        gfx_print_int(pct);
        gfx_print("%");

        gfx_draw_rect(BAR_X, BAR_Y, BAR_W, BAR_H, WHITE);
        gfx_fill_rect(BAR_X + BAR_W, BAR_Y + 6, 4, BAR_H - 12, WHITE);
        int fill = (BAR_W - 4) * pct / 100;
        gfx_fill_rect(BAR_X + 2, BAR_Y + 2, fill, BAR_H - 4, c);
        gfx_fill_rect(BAR_X + 2 + fill, BAR_Y + 2, BAR_W - 4 - fill, BAR_H - 4, BLACK);
        shown_pct = pct;
    }
    // The reading wanders by a few mV; only redraw on a real change.
    if (mv - shown_mv >= 10 || shown_mv - mv >= 10) {
        gfx_fill_rect(0, 30, LCD_WIDTH, 20, BLACK);
        gfx_set_font(&FreeMono9pt7b);
        gfx_set_text_color(GREY);
        gfx_set_cursor_no_height(6, 44);
        gfx_print_int(mv);
        gfx_print("mV");
        shown_mv = mv;
    }
    if ((int)chg != last_charger) {
        gfx_fill_rect(0, 84, LCD_WIDTH, 20, BLACK);
        gfx_set_font(&FreeMono9pt7b);
        gfx_set_text_color(chg ? CYAN : GREY);
        gfx_set_cursor_no_height(chg ? 2 : 13, 98);
        gfx_print(chg ? "CHARGING" : "BATTERY");
        last_charger = chg;
    }

    LOG("battery %d mV %d%%, charger %s", mv, pct, chg ? "yes" : "no");
    last_battery = hal_systick();
}

static void show_event(const char *name, uint16_t color) {
    gfx_fill_rect(0, 112, LCD_WIDTH, 48, BLACK);
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(color);
    gfx_set_cursor_no_height(2, 128);
    gfx_print(name);
    gfx_set_text_color(GREY);
    gfx_set_cursor_no_height(2, 150);
    gfx_print("n=");
    gfx_print_int(presses);
}

void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    power_init();
    button_init();
    vibrate_init();
    draw_battery();
    show_event("touch me", GREY);
    LOG("status: tap = 1 buzz, long = 2, very long = 3");
}

void app_update(void) {
    switch (button_poll()) {
    case BUTTON_PRESS:
        presses++;
        show_event("tap", WHITE);
        LOG("tap");
        vibrate_ms(80);
        break;
    case BUTTON_LONG:
        presses++;
        show_event("long", YELLOW);
        LOG("long press");
        vibrate_pulses(2, 80, 120);
        break;
    case BUTTON_VERY_LONG:
        show_event("v.long", RED);
        LOG("very long press");
        vibrate_pulses(3, 80, 120);
        break;
    default:
        break;
    }
    // Battery every second (hal_systick() counts 625 us), or at once when the
    // charger is plugged or unplugged.
    if (hal_systick() - last_battery >= 1600 || (int)charger_present() != last_charger)
        draw_battery();
    WaitMs(20);
}
