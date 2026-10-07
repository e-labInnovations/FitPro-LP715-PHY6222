// LP715 panel-ID probe: reads the LCD controller ID the same way the stock
// firmware does (0x1fff2cbc): bit-banged 3-wire read on the shared SDA line.
// Measured on the LP715: RDDID = 98 50 00 (JD9850-type).
#include "app.h"

#define P_RST  LCD_RST
#define P_DC   LCD_DC
#define P_CS   LCD_CS
#define P_SDA  LCD_SDA
#define P_SCL  LCD_SCL

static void out(gpio_pin_e p, uint8_t v) {
    hal_gpio_fmux(p, Bit_DISABLE);
    hal_gpio_pin_init(p, GPIO_OUTPUT);
    hal_gpio_write(p, v);
}

// Send one command byte, give `dummy` extra clocks, then read n bytes.
static void lcd_read(uint8_t cmd, int dummy, uint8_t *buf, int n) {
    out(P_SCL, 1);
    out(P_SDA, 1);
    out(P_CS, 1);
    hal_gpio_write(P_DC, 0);
    WaitUs(10);
    hal_gpio_write(P_CS, 0);
    WaitUs(10);
    for (int i = 0; i < 8; i++) {
        hal_gpio_write(P_SCL, 0);
        hal_gpio_write(P_SDA, (cmd & 0x80) != 0);
        WaitUs(11);
        hal_gpio_write(P_SCL, 1);
        WaitUs(11);
        cmd <<= 1;
    }
    hal_gpio_write(P_DC, 1);
    hal_gpio_pin_init(P_SDA, GPIO_INPUT);
    hal_gpio_pull_set(P_SDA, GPIO_PULL_UP);
    for (int i = 0; i < dummy; i++) {
        hal_gpio_write(P_SCL, 0);
        WaitUs(10);
        hal_gpio_write(P_SCL, 1);
    }
    for (int b = 0; b < n; b++) {
        uint8_t v = 0;
        for (int i = 0; i < 8; i++) {
            v <<= 1;
            WaitUs(10);
            hal_gpio_write(P_SCL, 0);
            if (hal_gpio_read(P_SDA))
                v |= 1;
            WaitUs(10);
            hal_gpio_write(P_SCL, 1);
        }
        buf[b] = v;
    }
    WaitUs(10);
    hal_gpio_write(P_CS, 1);
    hal_gpio_pull_set(P_SDA, GPIO_FLOATING);
    out(P_SDA, 1);
}

static void lcd_reset(void) {
    out(P_DC, 1);
    out(P_CS, 1);
    out(P_RST, 1);
    hal_gpio_write(P_RST, 0);
    WaitMs(200);
    hal_gpio_write(P_RST, 1);
    WaitMs(200);
}

static const char *guess(const uint8_t *id) {
    if (id[0] == 0x00 && id[1] == 0x91) return "GC91xx (stock profiles 0/2/5)";
    if (id[0] == 0x33 && id[1] == 0x30) return "NV3023/NV3025 (stock profiles 1/3)";
    if (id[0] == 0x98 && id[1] == 0x50) return "JD9850 (stock profile 6)";
    if ((id[0] == 0x7c || id[0] == 0xfc) && id[1] == 0x89 && id[2] == 0xf0) return "ST7735S (stock profile 4)";
    if ((id[0] == 0x1c && id[1] == 0x80) || ((id[0] & 0x7f) == 0x06 && id[1] == 0xec) || (id[0] == 0x83 && id[1] == 0x76))
        return "ST7735S-compatible (stock profile 4)";
    return "unknown";
}

void app_init(void) {
    LOG("LP715 panel-ID probe");
}

void app_update(void) {
    uint8_t id[3], raw[4], d1[1], d2[1], d3[1];

    lcd_reset();
    lcd_read(0x04, 1, id, 3);      // exactly what the stock firmware does
    lcd_read(0x04, 0, raw, 4);     // same, no dummy clock, one extra byte
    lcd_read(0xda, 1, d1, 1);
    lcd_read(0xdb, 1, d2, 1);
    lcd_read(0xdc, 1, d3, 1);
    LOG("RDDID(04) = %02x %02x %02x -> %s", id[0], id[1], id[2], guess(id));
    LOG("raw 04 no-dummy = %02x %02x %02x %02x", raw[0], raw[1], raw[2], raw[3]);
    LOG("DA DB DC = %02x %02x %02x", d1[0], d2[0], d3[0]);
    WaitMs(1000);
}
