#include "led/led.h"
#include "board.h"

void led_init(void) {
    hal_gpio_pin_init(HR_LED, GPIO_OUTPUT);
    hal_gpio_write(HR_LED, 1);   // off
}

void led_set(bool on) {
    hal_gpio_write(HR_LED, on ? 0 : 1);
}
