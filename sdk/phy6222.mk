# Shared build rules for PHY6222 examples. An example's Makefile sets TARGET and
# SRC, then includes this file:
#
#   TARGET = blink
#   SRC    = main.c
#   include ../../sdk/phy6222.mk
#
# Optional: LIB_SRC (extra sources from lib/), BLE=1 (link the BLE host stack).
# Output: _build/$(TARGET).hex — flash it with `rdwr_phy62x2.py ... wh`.

ROOT    := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
SDK     ?= /opt/phy6222_sdk
CORE    := $(ROOT)/lib/core
BUILD_DIR = _build

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

ARCH_FLAGS = -mcpu=cortex-m0 -mthumb

DEFINES = \
	-D__GCC -DARMCM0 -DPHY_MCU_TYPE=MCU_BUMBEE_M0 \
	-DCFG_SLEEP_MODE=PWR_MODE_SLEEP -DDEBUG_INFO=3 \
	-DMTU_SIZE=240 -DMAX_NUM_LL_CONN=1 -DDEF_GAPBOND_MGR_ENABLE=0 \
	-DTEST_RTC_DELTA=1 -DLL_DEBUG_NONE=1 -DCLK_16M_ONLY=1 -DSTACK_MAX_SRAM=1 \
	-DBROADCASTER_CFG=0x01 -DOBSERVER_CFG=0x02 -DPERIPHERAL_CFG=0x04 -DCENTRAL_CFG=0x08 \
	-DHOST_CONFIG=0x04 -DOSAL_CBTIMER_NUM_TASKS=1 -DENABLE_LOG_ROM_=0 \
	-DOSALMEM_METRICS=0 -DUSE_FS=0 -DOTA_TYPE=OTA_TYPE_NONE

SDK_INC_DIRS = \
	misc misc/CMSIS/include misc/CMSIS/device/phyplus \
	components/arch/cm0 components/inc components/osal/include components/common \
	components/ble/controller components/ble/include components/ble/hci components/ble/host \
	components/profiles/Roles \
	components/driver/adc components/driver/clock components/driver/dma components/driver/flash \
	components/driver/gpio components/driver/i2c components/driver/log components/driver/pwm \
	components/driver/pwrmgr components/driver/spi components/driver/spiflash \
	components/driver/timer components/driver/uart components/driver/watchdog

INCLUDES = -I. -I$(CORE) -I$(ROOT)/lib $(addprefix -I$(SDK)/,$(SDK_INC_DIRS))

CFLAGS  = $(ARCH_FLAGS) -Os -g3 -W -Wall -std=gnu99 \
	-ffunction-sections -fdata-sections -funsigned-char -funsigned-bitfields -fms-extensions \
	-fno-diagnostics-show-caret -MMD -MP $(DEFINES) $(INCLUDES)

LDSCRIPT = $(SDK)/misc/phy6222.ld
LDFLAGS  = $(ARCH_FLAGS) --static -nostartfiles -nostdlib -specs=nosys.specs \
	-Wl,--gc-sections -Wl,--script=$(LDSCRIPT) \
	-Wl,--just-symbols=$(SDK)/misc/bb_rom_sym_m0.gcc \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map
LDLIBS   = -Wl,--start-group -lgcc -lm -lnosys -Wl,--end-group

SDK_SRC = \
	lib/rf/patch.c lib/sec/phy_sec_ext.c lib/sec/aes.c \
	lib/ble_controller/rf_phy_driver.c \
	components/driver/clock/clock.c components/driver/flash/flash.c \
	components/driver/gpio/gpio.c components/driver/spi/spi.c components/driver/dma/dma.c \
	components/driver/pwrmgr/pwrmgr.c components/driver/timer/timer.c \
	components/driver/uart/uart.c components/driver/watchdog/watchdog.c \
	components/driver/log/my_printf.c misc/jump_table.c \
	misc/CMSIS/device/phyplus/phy6222_cstart.c misc/CMSIS/device/phyplus/phy6222_vectors.c

ifdef BLE
SDK_SRC += $(addprefix lib/ble_host/, \
	att_client.c att_server.c att_util.c gap_centdevmgr.c gap_centlinkmgr.c gap_configmgr.c \
	gap_devmgr.c gap_linkmgr.c gap_peridevmgr.c gap_perilinkmgr.c gap_task.c gatt_client.c \
	gatt_server.c gatt_task.c gatt_uuid.c l2cap_if.c l2cap_task.c l2cap_util.c linkdb.c \
	sm_intpairing.c sm_mgr.c sm_pairing.c smp.c sm_rsppairing.c sm_task.c)
endif

SDK_ASM  = misc/CMSIS/device/phyplus/phy6222_start.s
CORE_SRC = main.c osal_init.c osal_tasks.c syscalls.c

# Objects are namespaced by origin so same-named files cannot collide.
OBJS = \
	$(addprefix $(BUILD_DIR)/app/,  $(SRC:.c=.o)) \
	$(addprefix $(BUILD_DIR)/lib/,  $(LIB_SRC:.c=.o)) \
	$(addprefix $(BUILD_DIR)/core/, $(CORE_SRC:.c=.o)) \
	$(addprefix $(BUILD_DIR)/sdk/,  $(SDK_SRC:.c=.o)) \
	$(addprefix $(BUILD_DIR)/sdk/,  $(SDK_ASM:.s=.o))

all: $(BUILD_DIR)/$(TARGET).hex
	@$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(BUILD_DIR)/app/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lib/%.o: $(ROOT)/lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/core/%.o: $(CORE)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# SDK code is vendor code: keep its warnings out of our output.
$(BUILD_DIR)/sdk/%.o: $(SDK)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -w -c $< -o $@

$(BUILD_DIR)/sdk/%.o: $(SDK)/%.s
	@mkdir -p $(dir $@)
	@$(CC) $(ARCH_FLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

.PHONY: all clean
clean:
	rm -rf $(BUILD_DIR)
