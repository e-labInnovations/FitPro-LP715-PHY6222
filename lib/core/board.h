// FitPro LP715 pin map. Every entry here was read out of the stock firmware
// (see README); unconfirmed pins are not listed.
#ifndef BOARD_H
#define BOARD_H

#include "gpio.h"

// Display: 80x160, JD9850-type controller (ID 98 50 00), 3-wire SPI on SPI0.
// SDA is bidirectional — the panel ID is read back on it.
#define LCD_SCL   GPIO_P34
#define LCD_SDA   GPIO_P32
#define LCD_CS    GPIO_P31
#define LCD_DC    GPIO_P25
#define LCD_RST   GPIO_P24

// Active low: the stock firmware drives all four LOW with the screen on and
// HIGH with it off. One of them is the backlight; the others' roles are not
// pinned down yet.
#define LCD_PWR_A GPIO_P01
#define LCD_PWR_B GPIO_P02
#define LCD_PWR_C GPIO_P16
#define LCD_PWR_D GPIO_P17

// UART log (also the flashing port).
#define UART_TX   GPIO_P09
#define UART_RX   GPIO_P10

#endif
