#include "vibrate/vibrate.h"
#include "board.h"
#include "clock.h"

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
