#include "led/led.h"
#include "board.h"

void led_init(void) {
    hal_gpioretention_register(HR_LED);   // output that keeps its level in sleep
    hal_gpio_write(HR_LED, 1);   // off
}

void led_set(bool on) {
    hal_gpio_write(HR_LED, on ? 0 : 1);
}
