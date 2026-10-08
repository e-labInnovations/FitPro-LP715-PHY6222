// System bring-up for the LP715: clocks, power manager, RF/BLE controller init,
// UART log on P9/P10, then OSAL, which calls app_init()/app_update().
// Adapted from amir1387aht/phy6222_smartwatch source/main.c (MIT).
#include "gpio.h"
#include "clock.h"
#include "global_config.h"
#include "jump_function.h"
#include "pwrmgr.h"
#include "mcu.h"
#include "log.h"
#include "rf_phy_driver.h"
#include "flash.h"

extern void init_config(void);
extern void app_osal_init(void);

// OSAL heap. The BLE host allocates its connection buffers here; the stock
// firmware gives it 8 KB (osal_mem_set_heap at 0x1fff627a).
#ifdef CFG_BLE
#define LARGE_HEAP_SIZE (8 * 1024)
#else
#define LARGE_HEAP_SIZE (4 * 1024)
#endif
ALIGN4_U8 g_largeHeap[LARGE_HEAP_SIZE];
volatile uint8 g_clk32K_config;
volatile sysclk_t g_spif_clk_config;

#ifdef CFG_BLE
#include "OSAL_Memory.h"
#include "ll.h"
#include "ll_def.h"
#include "ll_buf.h"

// Link-layer connection buffers, set up as the stock firmware does at
// 0x1fff628c: one connection, 3 TX and 3 RX packets per event, BLE 5.1 packets,
// a 2680-byte buffer. This SDK copy lacks the packet-length constants the ROM
// uses, so each slot is sized generously (2720 bytes in all).
#define BLE_MAX_CONN       1
#define BLE_PKTS_PER_EVT_TX 3
#define BLE_PKTS_PER_EVT_RX 3
#define BLE_PKT_SLOT       272
#define BLE_CONN_BUF_SIZE  \
    (BLE_MAX_CONN * (BLE_PKTS_PER_EVT_TX * 2 + BLE_PKTS_PER_EVT_RX + 1) * BLE_PKT_SLOT)
ALIGN4_U8 g_pConnectionBuffer[BLE_CONN_BUF_SIZE];
llConnState_t pConnContext[BLE_MAX_CONN];

static void ble_mem_init_config(void) {
    osal_mem_set_heap((osalMemHdr_t *)g_largeHeap, LARGE_HEAP_SIZE);
    LL_InitConnectContext(pConnContext, g_pConnectionBuffer, BLE_MAX_CONN, BLE_PKTS_PER_EVT_TX,
                          BLE_PKTS_PER_EVT_RX, BLE_PKT_VERSION_5_1);
}
#endif

static void hal_low_power_io_init(void) {
    // Everything floats except the UART lines; examples configure what they use.
    const ioinit_cfg_t ioInit[] = {
        {GPIO_P00, GPIO_FLOATING}, {GPIO_P01, GPIO_FLOATING}, {GPIO_P02, GPIO_FLOATING},
        {GPIO_P03, GPIO_FLOATING}, {GPIO_P07, GPIO_FLOATING}, {GPIO_P09, GPIO_PULL_UP},
        {GPIO_P10, GPIO_PULL_UP},  {GPIO_P11, GPIO_FLOATING}, {GPIO_P14, GPIO_FLOATING},
        {GPIO_P15, GPIO_FLOATING}, {GPIO_P16, GPIO_FLOATING}, {GPIO_P17, GPIO_FLOATING},
        {GPIO_P18, GPIO_FLOATING}, {GPIO_P20, GPIO_FLOATING}, {GPIO_P23, GPIO_FLOATING},
        {GPIO_P24, GPIO_FLOATING}, {GPIO_P25, GPIO_FLOATING}, {GPIO_P26, GPIO_FLOATING},
        {GPIO_P31, GPIO_FLOATING}, {GPIO_P32, GPIO_FLOATING}, {GPIO_P33, GPIO_FLOATING},
        {GPIO_P34, GPIO_FLOATING},
    };
    for (uint8_t i = 0; i < sizeof(ioInit) / sizeof(ioInit[0]); i++)
        hal_gpio_pull_set(ioInit[i].pin, ioInit[i].type);

    DCDC_CONFIG_SETTING(0x0a);
    DCDC_REF_CLK_SETTING(1);
    DIG_LDO_CURRENT_SETTING(1);
    hal_pwrmgr_RAM_retention(RET_SRAM0 | RET_SRAM1 | RET_SRAM2);
    hal_pwrmgr_RAM_retention_set();
    subWriteReg(0x4000f014, 26, 26, 1); // hal_pwrmgr_LowCurrentLdo_enable()
}

static void hal_rfphy_init(void) {
    g_rfPhyTxPower = RF_PHY_TX_POWER_0DBM;
    g_rfPhyPktFmt = PKT_FMT_BLE1M;
    g_rfPhyFreqOffSet = RF_PHY_FREQ_FOFF_00KHZ;
    XTAL16M_CAP_SETTING(0x09);
    XTAL16M_CURRENT_SETTING(0x01);
    hal_rc32k_clk_tracking_init();
    {
        // hal_rom_boot_init(): efuse, then the ROM's ble_main at 0xa2e1
        extern void efuse_init(void);
        efuse_init();
        typedef void (*rom_fn)(void);
        ((rom_fn)0xa2e1)();
    }
#ifdef CFG_BLE
    ble_mem_init_config();
    hal_rfPhyFreqOff_Set();
#endif
    NVIC_SetPriority((IRQn_Type)BB_IRQn, IRQ_PRIO_REALTIME);
    NVIC_SetPriority((IRQn_Type)TIM1_IRQn, IRQ_PRIO_HIGH); // ll_EVT
    NVIC_SetPriority((IRQn_Type)TIM2_IRQn, IRQ_PRIO_HIGH); // OSAL_TICK
    NVIC_SetPriority((IRQn_Type)TIM4_IRQn, IRQ_PRIO_HIGH); // LL_EXA_ADV
}

static void hal_init(void) {
    hal_low_power_io_init();
    clk_init(g_system_clk);
    hal_rtc_clock_config((CLK32K_e)g_clk32K_config);
    hal_pwrmgr_init();
    hal_spif_cache_init(SYS_CLK_DLL_64M, XFRD_FCMD_READ_DUAL);
    hal_gpio_init();
    LOG_INIT();
}

int main(void) {
    g_system_clk = CFG_SYS_CLK;   // set by SYS_CLK in sdk/phy6222.mk
    g_clk32K_config = CLK_32K_RCOSC;
    drv_irq_init();
    init_config();
    extern void ll_patch_slave(void);
    ll_patch_slave();
    hal_rfphy_init();
    hal_init();
    app_osal_init();
    return 0;
}
