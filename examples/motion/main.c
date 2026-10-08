// Motion switch test (P18, a ball/tilt switch). Shows the number of shakes —
// bursts of switch bounces, grouped like the stock firmware does — with the
// raw level changes and the current level underneath. The HR LED flashes
// once per shake.
#include "app.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "fonts/FreeSans12pt7b.h"
#include "fonts/FreeMono9pt7b.h"
#include "led/led.h"
#include "motion/motion.h"

#define WHITE RGB565(255, 255, 255)
#define BLACK RGB565(0, 0, 0)
#define GREY  RGB565(90, 90, 90)
#define GREEN RGB565(0, 220, 80)

static int shakes, shown_shakes = -1, shown_level = -1;
static uint32_t shown_edges = (uint32_t)-1, last_draw, led_off_at;

static void draw(void) {
    if (shakes != shown_shakes) {
        gfx_fill_rect(0, 22, LCD_WIDTH, 30, BLACK);
        gfx_set_font(&FreeSans12pt7b);
        gfx_set_text_color(WHITE);
        gfx_set_cursor_no_height(6, 46);
        gfx_print_int(shakes);
        shown_shakes = shakes;
    }
    gfx_set_font(&FreeMono9pt7b);
    if (motion_edges() != shown_edges) {
        gfx_fill_rect(0, 70, LCD_WIDTH, 20, BLACK);
        gfx_set_text_color(GREY);
        gfx_set_cursor_no_height(2, 84);
        gfx_print("raw ");
        gfx_print_int(motion_edges());
        shown_edges = motion_edges();
    }
    if (motion_level() != shown_level) {
        gfx_fill_rect(0, 96, LCD_WIDTH, 20, BLACK);
        gfx_set_text_color(motion_level() ? GREEN : GREY);
        gfx_set_cursor_no_height(2, 110);
        gfx_print(motion_level() ? "P18 = 1" : "P18 = 0");
        shown_level = motion_level();
    }
}

void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    led_init();
    motion_init();
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(GREY);
    gfx_set_cursor_no_height(2, 14);
    gfx_print("shakes");
    draw();
    LOG("motion: P18 idle level %d", motion_level());
}

void app_update(void) {
    uint32_t now = hal_systick();
    if (motion_poll()) {
        shakes++;
        led_set(true);
        led_off_at = now + 64;   // ~40 ms
        LOG("shake %d (raw %d)", shakes, (int)motion_edges());
    }
    if (led_off_at && now >= led_off_at) {
        led_set(false);
        led_off_at = 0;
    }
    if (now - last_draw >= 160) {   // redraw at most ~10x a second
        draw();
        last_draw = now;
    }
    WaitUs(1000);
}
