// gfx test: text in both fonts, a raw and an RLE image from tools/img2c.py,
// filled shapes, and an arc that sweeps round like a progress ring.
#include "app.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "fonts/FreeSans12pt7b.h"
#include "fonts/FreeMono9pt7b.h"
#include "gradient.h"   // 40x40 raw:  python3 tools/img2c.py gradient.png --name gradient
#include "badge.h"      // 40x40 RLE:  python3 tools/img2c.py badge.png --name badge --rle

#define WHITE  RGB565(255, 255, 255)
#define BLACK  RGB565(0, 0, 0)
#define ORANGE RGB565(255, 140, 0)
#define TEAL   RGB565(0, 160, 160)
#define GREY   RGB565(60, 60, 60)

#define RING_X 40
#define RING_Y 134
#define RING_R 22

static int angle;

// FreeMono9pt7b is 11 px per character; "100" fits inside the inner circle.
static void draw_percent(int pct) {
    char s[4];
    int n = 0;
    if (pct >= 100)
        s[n++] = '0' + pct / 100;
    if (pct >= 10)
        s[n++] = '0' + pct / 10 % 10;
    s[n++] = '0' + pct % 10;
    s[n] = 0;
    gfx_fill_rect(RING_X - 17, RING_Y - 7, 34, 14, BLACK);
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(WHITE);
    gfx_set_cursor_no_height(RING_X - n * 11 / 2, RING_Y + 5);   // baseline
    gfx_print(s);
}

void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    uint32_t t = hal_systick();

    gfx_set_font(&FreeSans12pt7b);
    gfx_set_text_color(WHITE);
    gfx_set_cursor_no_height(4, 20);   // baseline
    gfx_print("LP715");
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(ORANGE);
    gfx_set_cursor_no_height(7, 36);
    gfx_print("JD9850");

    lcd_draw_image(0, 42, GRADIENT_WIDTH, GRADIENT_HEIGHT, gradient_data);
    lcd_draw_image_rle(40, 42, BADGE_WIDTH, BADGE_HEIGHT, badge_rle, BADGE_RLE_RUNS);

    gfx_fill_circle(12, 98, 10, TEAL);
    gfx_fill_round_rect(28, 88, 22, 20, 5, ORANGE);
    gfx_fill_triangle(56, 108, 67, 88, 78, 108, WHITE);

    gfx_draw_circle(RING_X, RING_Y, RING_R, GREY);
    gfx_draw_circle(RING_X, RING_Y, RING_R - 4, GREY);
    draw_percent(0);
    // hal_systick() counts 625 us BLE slots
    LOG("gfx test drawn in %d ms", (int)((hal_systick() - t) * 625 / 1000));
}

void app_update(void) {
    int next = angle + 6;
    gfx_draw_arc(RING_X, RING_Y, RING_R - 1, angle, next, 3, TEAL);
    angle = next;
    draw_percent(angle * 100 / 360);
    if (angle >= 360) {
        LOG("ring full");
        WaitMs(1000);
        gfx_draw_arc(RING_X, RING_Y, RING_R - 1, 0, 360, 3, BLACK);
        angle = 0;
    }
    WaitMs(30);
}
