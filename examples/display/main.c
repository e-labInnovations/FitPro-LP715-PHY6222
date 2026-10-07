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

static void draw_bands(void) {
    for (int i = 0; i < 8; i++)
        lcd_fill_rect(0, i * BAND_H, LCD_WIDTH, BAND_H, bands[i]);
    uint16_t w = RGB565(255, 255, 255);
    lcd_fill_rect(0, 0, LCD_WIDTH, 1, w);
    lcd_fill_rect(0, LCD_HEIGHT - 1, LCD_WIDTH, 1, w);
    lcd_fill_rect(0, 0, 1, LCD_HEIGHT, w);
    lcd_fill_rect(LCD_WIDTH - 1, 0, 1, LCD_HEIGHT, w);
}

void app_init(void) {
    LOG("display test: init");
    lcd_init();
    uint32_t t = hal_systick();
    draw_bands();
    // hal_systick() counts 625 us BLE slots
    LOG("bands drawn in %d ms: red green blue white cyan magenta yellow black, top to bottom",
        (int)((hal_systick() - t) * 625 / 1000));
}

// Repaints rows y..y+h-1 of the box's column with the bands behind it.
static void restore_bands(int y, int h) {
    while (h > 0) {
        int band = y / BAND_H;
        int n = (band + 1) * BAND_H - y;
        if (n > h)
            n = h;
        lcd_fill_rect((LCD_WIDTH - BOX) / 2, y, BOX, n, bands[band]);
        y += n;
        h -= n;
    }
}

void app_update(void) {
    // Draw the box at its new place first, then repaint only the strip it
    // left. Erasing the whole box before redrawing it flickers whenever the
    // panel's refresh scan passes between the two writes.
    int old_y = box_y;
    box_y = (box_y + 4) % (LCD_HEIGHT - BOX);
    lcd_fill_rect((LCD_WIDTH - BOX) / 2, box_y, BOX, BOX, RGB565(128, 128, 128));
    if (box_y > old_y) {
        restore_bands(old_y, box_y - old_y);
    } else {
        restore_bands(old_y, BOX);
        LOG("box wrapped");
    }
    WaitMs(50);
}
