// Display test: colour bands top to bottom (red, green, blue, white, cyan,
// magenta, yellow, black) inside a white border, then a square moving down.
#include "app.h"
#include "display/jd9850.h"

static const uint16_t bands[] = {
    RGB565(255, 0, 0), RGB565(0, 255, 0), RGB565(0, 0, 255), RGB565(255, 255, 255),
    RGB565(0, 255, 255), RGB565(255, 0, 255), RGB565(255, 255, 0), RGB565(0, 0, 0),
};

#define BAND_H (LCD_HEIGHT / 8)
#define BOX    16

static int box_y;

static void draw_bands(void)
{
    for (int i = 0; i < 8; i++)
        lcd_fill_rect(0, i * BAND_H, LCD_WIDTH, BAND_H, bands[i]);
    uint16_t w = RGB565(255, 255, 255);
    lcd_fill_rect(0, 0, LCD_WIDTH, 1, w);
    lcd_fill_rect(0, LCD_HEIGHT - 1, LCD_WIDTH, 1, w);
    lcd_fill_rect(0, 0, 1, LCD_HEIGHT, w);
    lcd_fill_rect(LCD_WIDTH - 1, 0, 1, LCD_HEIGHT, w);
}

void app_init(void)
{
    LOG("display test: init");
    lcd_init();
    uint32_t t = hal_systick();
    draw_bands();
    LOG("bands drawn in %d ms: red green blue white cyan magenta yellow black, top to bottom",
        (int)(hal_systick() - t));
}

void app_update(void)
{
    // Restore the band under the old box, then draw the box lower down.
    lcd_fill_rect((LCD_WIDTH - BOX) / 2, box_y, BOX, BOX, bands[box_y / BAND_H]);
    if (box_y / BAND_H != (box_y + BOX - 1) / BAND_H)
        lcd_fill_rect((LCD_WIDTH - BOX) / 2, (box_y / BAND_H + 1) * BAND_H, BOX,
                      box_y + BOX - (box_y / BAND_H + 1) * BAND_H, bands[box_y / BAND_H + 1]);
    box_y = (box_y + 4) % (LCD_HEIGHT - BOX);
    lcd_fill_rect((LCD_WIDTH - BOX) / 2, box_y, BOX, BOX, RGB565(128, 128, 128));
    if (box_y == 0)
        LOG("box wrapped");
    WaitMs(50);
}
