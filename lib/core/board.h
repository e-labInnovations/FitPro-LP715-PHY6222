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

// Backlight: four active-low GPIOs, each sinking part of the LED current
// through its own resistor. All LOW = full brightness (what the stock firmware
// does with the screen on), all HIGH = off. Measured on the watch: P17 alone
// lights the screen visibly; P01, P02 or P16 alone look dark but each adds
// brightness in combination.
#define LCD_BL_P01 GPIO_P01
#define LCD_BL_P02 GPIO_P02
#define LCD_BL_P16 GPIO_P16
#define LCD_BL_P17 GPIO_P17

// Touch key: active high, input with pull-down (stock: interrupt, polled
// every 50 ms while held; long press 1.55 s, very long 2.55 s).
#define BUTTON    GPIO_P11

// Vibration motor: active high (stock pattern engine at 0x1fff7370).
#define VIBRATOR  GPIO_P03

// Battery: ADC channel ADC_CH2P_P14, high-resolution (0-800 mV) through a
// 1/5.5 divider. Measured 3.71 V on battery, ~3.9 V while charging.
#define BAT_ADC   GPIO_P14

// Charger detect: active high, input with pull-down. Only valid while
// CHG_CTL is LOW — on the board P15 follows P23 when no charger is attached.
#define CHG_DET   GPIO_P15

// Charger-related output, coupled to CHG_DET. The stock firmware keeps it LOW;
// HIGH while charging lowers the battery reading by ~110 mV. Role not pinned
// down yet — keep LOW.
#define CHG_CTL   GPIO_P23

// Factory-test strap: strong pull-up, read once at boot; LOW enters the stock
// firmware's test mode.
#define TEST_STRAP GPIO_P07

// UART log (also the flashing port).
#define UART_TX   GPIO_P09
#define UART_RX   GPIO_P10

#endif
