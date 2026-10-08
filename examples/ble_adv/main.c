// First BLE test: advertise as "LP715" and accept a connection (no custom
// services yet). The screen shows the state; connect with nRF Connect.
#include "app.h"
#include "ble/ble.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "fonts/FreeMono9pt7b.h"

#define WHITE RGB565(255, 255, 255)
#define BLACK RGB565(0, 0, 0)
#define GREY  RGB565(90, 90, 90)
#define BLUE  RGB565(40, 120, 255)
#define GREEN RGB565(0, 220, 80)

static int shown = -1;
void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(WHITE);
    gfx_set_cursor_no_height(13, 20);
    gfx_print("LP715");
    ble_set_name("LP715");
}

void app_update(void) {
    ble_state_t s = ble_state();
    if ((int)s == shown)
        return;
    shown = s;
    gfx_fill_rect(0, 60, LCD_WIDTH, 40, BLACK);
    gfx_set_cursor_no_height(2, 84);
    if (s == BLE_CONNECTED) {
        gfx_set_text_color(GREEN);
        gfx_print("connected");
    } else if (s == BLE_ADVERTISING) {
        gfx_set_text_color(BLUE);
        gfx_print("advert.");
    } else {
        gfx_set_text_color(GREY);
        gfx_print("idle");
    }
}
