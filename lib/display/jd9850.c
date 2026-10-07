#include "jd9850.h"
#include "board.h"
#include "gpio.h"
#include "spi.h"
#include "clock.h"

// Stock profile 6 init sequence, as {command, data length, data...}.
// The stock table stores it as 16-bit words (0xff = command follows,
// 0xfe = delay); this is the same sequence decoded.
static const uint8_t init_seq[] = {
    0xDF, 2, 0x98, 0x50,                         // unlock (vendor password)
    0xDE, 1, 0x00,                               // register page 0
    0xB2, 1, 0x17,
    0xB7, 2, 0x1E, 0x1E,
    0xB9, 3, 0x17, 0x03, 0x00,
    0xBB, 6, 0x6C, 0x55, 0xA2, 0xBB, 0x7F, 0xEF,
    0xC1, 5, 0x22, 0x1B, 0x6A, 0x1B, 0x6A,
    0xC3, 8, 0x01, 0xA8, 0xFE, 0x3F, 0x87, 0xF2, 0xA4, 0xA4,
    0xC4, 7, 0x00, 0x00, 0x50, 0x49, 0x44, 0x35, 0x16,
    0xC8, 32, 0x7F, 0x76, 0x71, 0x70, 0x34, 0x3A, 0x37, 0x37,   // gamma
              0x35, 0x31, 0x2B, 0x19, 0x11, 0x06, 0x00, 0x80,
              0x3F, 0x36, 0x31, 0x30, 0x34, 0x3A, 0x37, 0x37,
              0x35, 0x31, 0x2B, 0x19, 0x11, 0x06, 0x00, 0x80,
    0xD0, 4, 0x06, 0x84, 0x98, 0x0F,
    0xD3, 2, 0x13, 0xFF,
    0xD7, 2, 0x6A, 0xE0,
    0xDE, 1, 0x01,                               // register page 1
    0xB2, 2, 0x10, 0xA2,
    0xB7, 4, 0x19, 0x15, 0x1D, 0x20,
    0xC2, 3, 0x16, 0x00, 0xEE,
    0xC5, 2, 0x11, 0x00,
    0x2A, 4, 0x00, 0x00, 0x00, 0x4F,             // columns 0..79
    0x2B, 4, 0x00, 0x00, 0x00, 0x9F,             // rows 0..159
    0x35, 1, 0x00,                               // tearing-effect output on
    0x36, 1, 0xD0,                               // memory access control
    0x3A, 1, 0x05,                               // RGB565
};

// Driven HIGH by the stock firmware with the screen off and LOW with it on;
// one of them is the backlight.
static const gpio_pin_e power_pins[] = {LCD_PWR_A, LCD_PWR_B, LCD_PWR_C, LCD_PWR_D};

static hal_spi_t spi = {.spi_index = SPI0};

#define SSI_SR_BUSY 0x01
#define SSI_SR_TFNF 0x02
#define SSI_SR_TFE  0x04

static void spi_wait_idle(void)
{
    while (!(AP_SPI0->SR & SSI_SR_TFE) || (AP_SPI0->SR & SSI_SR_BUSY))
        ;
}

static inline void spi_put(uint8_t b)
{
    while (!(AP_SPI0->SR & SSI_SR_TFNF))
        ;
    AP_SPI0->DataReg = b;
}

static void out_pin(gpio_pin_e pin, uint8_t level)
{
    hal_gpio_fmux(pin, Bit_DISABLE);
    hal_gpio_pin_init(pin, GPIO_OUTPUT);
    hal_gpio_write(pin, level);
}

static void write_cmd(uint8_t cmd, const uint8_t *data, int n)
{
    hal_gpio_write(LCD_CS, 0);
    hal_gpio_write(LCD_DC, 0);
    spi_put(cmd);
    spi_wait_idle();
    hal_gpio_write(LCD_DC, 1);
    for (int i = 0; i < n; i++)
        spi_put(data[i]);
    spi_wait_idle();
    hal_gpio_write(LCD_CS, 1);
}

static void set_window(int x0, int y0, int x1, int y1)
{
    uint8_t c[4] = {x0 >> 8, x0, x1 >> 8, x1};
    uint8_t r[4] = {y0 >> 8, y0, y1 >> 8, y1};
    write_cmd(0x2A, c, 4);
    write_cmd(0x2B, r, 4);
}

// Starts a RAMWR (0x2C); the caller streams pixels then calls end_pixels().
static void begin_pixels(void)
{
    hal_gpio_write(LCD_CS, 0);
    hal_gpio_write(LCD_DC, 0);
    spi_put(0x2C);
    spi_wait_idle();
    hal_gpio_write(LCD_DC, 1);
}

static void end_pixels(void)
{
    spi_wait_idle();
    hal_gpio_write(LCD_CS, 1);
}

void lcd_init(void)
{
    for (unsigned i = 0; i < sizeof(power_pins) / sizeof(power_pins[0]); i++)
        out_pin(power_pins[i], 1);
    out_pin(LCD_CS, 1);
    out_pin(LCD_DC, 1);
    out_pin(LCD_RST, 1);

    spi_Cfg_t cfg = {
        .sclk_pin = LCD_SCL,
        .ssn_pin = GPIO_DUMMY,      // CS is driven by hand, as in the stock firmware
        .MOSI = LCD_SDA,
        .MISO = GPIO_DUMMY,
        .baudrate = 32000000,       // stock value; the SDK clamps it to pclk/2
        .spi_tmod = SPI_TXD,
        .spi_scmod = SPI_MODE0,
        .spi_dfsmod = SPI_8BIT,
        .int_mode = false,
        .force_cs = false,
        .evt_handler = NULL,
    };
    hal_spi_bus_init(&spi, cfg);

    hal_gpio_write(LCD_RST, 0);
    WaitMs(200);
    hal_gpio_write(LCD_RST, 1);
    WaitMs(200);

    for (unsigned i = 0; i < sizeof(init_seq);) {
        uint8_t cmd = init_seq[i], n = init_seq[i + 1];
        write_cmd(cmd, &init_seq[i + 2], n);
        i += 2 + n;
    }

    lcd_fill(0x0000);
    lcd_on();
}

void lcd_on(void)
{
    write_cmd(0x11, NULL, 0);   // sleep out
    WaitMs(20);
    write_cmd(0x29, NULL, 0);   // display on
    WaitMs(200);
    for (unsigned i = 0; i < sizeof(power_pins) / sizeof(power_pins[0]); i++)
        hal_gpio_write(power_pins[i], 0);
}

void lcd_off(void)
{
    for (unsigned i = 0; i < sizeof(power_pins) / sizeof(power_pins[0]); i++)
        hal_gpio_write(power_pins[i], 1);
    write_cmd(0x28, NULL, 0);   // display off
    WaitMs(120);
    write_cmd(0x10, NULL, 0);   // sleep in
}

void lcd_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0)
        return;
    set_window(x, y, x + w - 1, y + h - 1);
    begin_pixels();
    for (int i = w * h; i > 0; i--) {
        spi_put(color >> 8);
        spi_put(color);
    }
    end_pixels();
}

void lcd_fill(uint16_t color)
{
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void lcd_draw_bitmap(int x, int y, int w, int h, const uint16_t *pixels)
{
    set_window(x, y, x + w - 1, y + h - 1);
    begin_pixels();
    for (int i = 0; i < w * h; i++) {
        spi_put(pixels[i] >> 8);
        spi_put(pixels[i]);
    }
    end_pixels();
}
