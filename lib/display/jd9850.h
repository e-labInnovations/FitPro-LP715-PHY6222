// Driver for the LP715's 80x160 panel: a JD9850-type controller (ID 98 50 00)
// on SPI0, with CS and DC as GPIOs. The init sequence is the stock firmware's
// profile 6 (flash 0x38298). Pixels are RGB565, sent big-endian.
#ifndef JD9850_H
#define JD9850_H

#include "types.h"

#define LCD_WIDTH  80
#define LCD_HEIGHT 160

#define RGB565(r, g, b) ((uint16_t)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3)))

// All drawing calls clip to the screen; coordinates may be negative.
void lcd_init(void);
void lcd_on(void);
void lcd_off(void);
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcd_fill(uint16_t color);
void lcd_draw_pixel(int x, int y, uint16_t color);
// Draws w*h RGB565 pixels, row by row (as made by tools/img2c.py, raw).
void lcd_draw_image(int x, int y, int w, int h, const uint16_t *pixels);
// Same, reading rows `stride` pixels apart — draws part of a larger image.
void lcd_draw_image_ex(int x, int y, int w, int h, const uint16_t *pixels, int stride);
// Draws a run-length encoded image: `runs` pairs of {count, color}
// (tools/img2c.py --rle).
void lcd_draw_image_rle(int x, int y, int w, int h, const uint16_t *rle, int runs);

#endif
