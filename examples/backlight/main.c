// Backlight test: lights each of the four active-low backlight pins on its
// own (box N red), then all four, then none. On the LP715 only P17 alone
// lights the screen visibly; P01, P02 and P16 alone look dark, though turning
// any one of them off from full brightness dims the screen.
#include "app.h"
#include "display/jd9850.h"

static const struct {
    gpio_pin_e pin;
    const char *name;
} pins[] = {
    {LCD_BL_P01, "P01"}, {LCD_BL_P02, "P02"}, {LCD_BL_P16, "P16"}, {LCD_BL_P17, "P17"},
};

#define N (sizeof(pins) / sizeof(pins[0]))

// Box i red = pin i on; active == N lights all four.
static void draw_markers(int active)
{
    for (int i = 0; i < (int)N; i++)
        lcd_fill_rect(4 + i * 19, 4, 15, 15,
                      (i == active || active == (int)N) ? RGB565(255, 0, 0) : RGB565(64, 64, 64));
}

void app_init(void)
{
    lcd_init();
    for (int i = 0; i < 8; i++)
        lcd_fill_rect(0, 20 + i * 17, LCD_WIDTH, 17,
                      (const uint16_t[]){RGB565(255, 0, 0), RGB565(0, 255, 0), RGB565(0, 0, 255),
                                         RGB565(255, 255, 255), RGB565(0, 255, 255), RGB565(255, 0, 255),
                                         RGB565(255, 255, 0), RGB565(128, 128, 128)}[i]);
    draw_markers(-1);
    LOG("backlight test: each pin alone, then all four, then all off");
}

void app_update(void)
{
    // Each pin alone on (box N lit = only that pin LOW), then all four on.
    for (int only = 0; only <= (int)N; only++) {
        for (int i = 0; i < (int)N; i++)
            hal_gpio_write(pins[i].pin, (only == (int)N || i == only) ? 0 : 1);
        draw_markers(only);
        if (only < (int)N)
            LOG("box %d: only %s LOW (on)", only + 1, pins[only].name);
        else
            LOG("all four LOW (on)");
        WaitMs(2000);
    }
    // A dark gap so the start of the cycle is easy to spot.
    for (int i = 0; i < (int)N; i++)
        hal_gpio_write(pins[i].pin, 1);
    LOG("all off");
    WaitMs(2000);
}
