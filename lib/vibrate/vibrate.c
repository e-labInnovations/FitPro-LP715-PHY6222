#include "vibrate/vibrate.h"
#include "board.h"
#include "clock.h"

// hal_systick() counts 625 us BLE slots.
#define MS_TO_TICKS(ms) ((ms) * 8 / 5)

static bool running;
static uint32_t started, length;

void vibrate_init(void) {
    hal_gpio_pin_init(VIBRATOR, GPIO_OUTPUT);
    hal_gpio_write(VIBRATOR, 0);
}

void vibrate_ms(int ms) {
    hal_gpio_write(VIBRATOR, 1);
    WaitMs(ms);
    hal_gpio_write(VIBRATOR, 0);
}

void vibrate_pulses(int count, int on_ms, int off_ms) {
    for (int i = 0; i < count; i++) {
        if (i)
            WaitMs(off_ms);
        vibrate_ms(on_ms);
    }
}

void vibrate_start(int ms) {
    started = hal_systick();
    length = MS_TO_TICKS(ms);
    running = true;
    hal_gpio_write(VIBRATOR, 1);
}

void vibrate_poll(void) {
    if (running && hal_systick() - started >= length) {
        hal_gpio_write(VIBRATOR, 0);
        running = false;
    }
}
