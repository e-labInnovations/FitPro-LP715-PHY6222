TARGET    = ble_ctrl
SRC       = main.c
LIB_SRC   = display/jd9850.c display/gfx.c fonts/FreeMono9pt7b.c ble/services.c \
            power/power.c input/button.c vibrate/vibrate.c led/led.c motion/motion.c
SDK_EXTRA = components/driver/adc/adc.c
BLE       = 1
SYS_CLK   = SYS_CLK_XTAL_16M
SLEEP_MODE = PWR_MODE_SLEEP
include ../../sdk/phy6222.mk
