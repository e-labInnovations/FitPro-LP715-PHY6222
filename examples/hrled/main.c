// Heart-rate LED test: a heartbeat double blink at ~70 bpm on the LED on the
// back (P00, active low), with a heart on screen pulsing in step.
#include "app.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "led/led.h"

#define RED   RGB565(230, 20, 40)
#define DARK  RGB565(60, 0, 10)
#define BLACK RGB565(0, 0, 0)

static void heart(uint16_t color) {
    int cx = LCD_WIDTH / 2, cy = 70;
    gfx_fill_circle(cx - 11, cy, 12, color);
    gfx_fill_circle(cx + 11, cy, 12, color);
    gfx_fill_triangle(cx - 23, cy + 4, cx + 23, cy + 4, cx, cy + 32, color);
}

static void beat(int on_ms) {
    led_set(true);
    heart(RED);
    WaitMs(on_ms);
    led_set(false);
    heart(DARK);
}

void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    led_init();
    heart(DARK);
    LOG("hr led: heartbeat on P00 (active low)");
}

void app_update(void) {
    beat(80);
    WaitMs(120);
    beat(80);
    WaitMs(580);   // ~860 ms per beat, 70 bpm
}
