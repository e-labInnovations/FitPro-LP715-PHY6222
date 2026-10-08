#include "input/button.h"
#include "board.h"
#include "clock.h"

// hal_systick() counts 625 us BLE slots.
#define MS_TO_TICKS(ms) ((ms) * 8 / 5)
#define DEBOUNCE   MS_TO_TICKS(30)
#define LONG       MS_TO_TICKS(1500)
#define VERY_LONG  MS_TO_TICKS(2500)

static bool pressed;        // debounced state
static uint32_t edge_at;    // when the raw level last changed
static uint32_t down_at;    // when the debounced press began
static uint8_t fired;       // 0 none, 1 long, 2 very long sent for this press

void button_init(void) {
    hal_gpio_pin_init(BUTTON, GPIO_INPUT);
    hal_gpio_pull_set(BUTTON, GPIO_PULL_DOWN);
}

bool button_down(void) {
    return hal_gpio_read(BUTTON);
}

button_event_t button_poll(void) {
    static bool raw_last;
    uint32_t now = hal_systick();
    bool raw = button_down();

    if (raw != raw_last) {
        raw_last = raw;
        edge_at = now;
    }
    if (raw != pressed && now - edge_at >= DEBOUNCE) {
        pressed = raw;
        if (pressed) {
            down_at = now;
            fired = 0;
        } else if (fired == 0) {
            return BUTTON_PRESS;
        }
    }
    if (pressed) {
        uint32_t held = now - down_at;
        if (fired < 2 && held >= VERY_LONG) {
            fired = 2;
            return BUTTON_VERY_LONG;
        }
        if (fired < 1 && held >= LONG) {
            fired = 1;
            return BUTTON_LONG;
        }
    }
    return BUTTON_NONE;
}
