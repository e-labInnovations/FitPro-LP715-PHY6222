#include "power/power.h"
#include "board.h"
#include "adc.h"
#include "clock.h"

#define SAMPLES 32

static adc_Cfg_t cfg = {
    .channel = ADC_BIT(ADC_CH2P_P14),
    .is_continue_mode = FALSE,
    .is_differential_mode = 0x00,
    .is_high_resolution = ADC_BIT(ADC_CH2P_P14),   // 0-800 mV, as the stock firmware
};

static uint16_t samples[SAMPLES];
static uint8_t sample_n;
static volatile bool done;

static void adc_evt(adc_Evt_t *ev) {
    if (ev->type != HAL_ADC_EVT_DATA || ev->ch != ADC_CH2P_P14)
        return;
    sample_n = ev->size > SAMPLES ? SAMPLES : ev->size;
    for (int i = 0; i < sample_n; i++)
        samples[i] = ev->data[i];
    done = true;
}

void power_init(void) {
    // CHG_CTL LOW as the stock firmware keeps it; CHG_DET is only valid then.
    hal_gpio_pin_init(CHG_CTL, GPIO_OUTPUT);
    hal_gpio_write(CHG_CTL, 0);
    hal_gpio_pin_init(CHG_DET, GPIO_INPUT);
    hal_gpio_pull_set(CHG_DET, GPIO_PULL_DOWN);
    hal_adc_init();
}

int battery_mv(void) {
    done = false;
    if (hal_adc_config_channel(cfg, adc_evt) != 0 || hal_adc_start() != 0)
        return -1;
    for (int t = 0; t < 100 && !done; t++)
        WaitMs(1);
    hal_adc_stop();
    if (!done)
        return -1;
    // In this SDK hal_adc_value_cal() returns millivolts at the pin.
    int pin_mv = (int)hal_adc_value_cal(ADC_CH2P_P14, samples, sample_n, 1, 0);
    return pin_mv * 11 / 2;
}

// Resting Li-ion voltage -> percent, linear between points.
static const struct {
    int mv, pct;
} curve[] = {
    {4150, 100}, {4050, 90}, {3970, 80}, {3910, 70}, {3850, 60}, {3800, 50},
    {3760, 40},  {3720, 30}, {3680, 20}, {3620, 10}, {3500, 5},  {3300, 0},
};

int battery_percent(int mv) {
    int n = sizeof(curve) / sizeof(curve[0]);
    if (mv >= curve[0].mv)
        return 100;
    for (int i = 1; i < n; i++) {
        if (mv >= curve[i].mv)
            return curve[i].pct + (mv - curve[i].mv) * (curve[i - 1].pct - curve[i].pct) /
                                      (curve[i - 1].mv - curve[i].mv);
    }
    return 0;
}

bool charger_present(void) {
    return hal_gpio_read(CHG_DET);
}

// hal_systick() counts 625 us BLE slots.
#define MS_TO_TICKS(ms) ((ms) * 8 / 5)
#define CHARGE_PERIOD MS_TO_TICKS(10000)
#define CHARGE_SETTLE MS_TO_TICKS(300)

static charge_state_t state = CHARGE_NONE;
static bool measuring;
static uint32_t phase_at;
static int rest_mv = -1;

void charge_poll(void) {
    uint32_t now = hal_systick();
    if (!measuring) {
        // Start a measurement at once on a new charger, then every period.
        bool due = now - phase_at >= CHARGE_PERIOD || (state == CHARGE_NONE && charger_present());
        if (!due)
            return;
        hal_gpio_write(CHG_CTL, 0);
        measuring = true;
        phase_at = now;
        return;
    }
    if (now - phase_at < CHARGE_SETTLE)
        return;
    measuring = false;
    phase_at = now;
    rest_mv = battery_mv();
    if (!charger_present())
        state = CHARGE_NONE;
    else if (rest_mv >= CHARGE_FULL_MV)
        state = CHARGE_FULL;
    else if (state != CHARGE_FULL || rest_mv < CHARGE_FULL_MV - 100)
        state = CHARGE_ON;   // a full cell only restarts once it has sagged
    hal_gpio_write(CHG_CTL, state == CHARGE_ON);
}

charge_state_t charge_state(void) {
    return state;
}

int charge_rest_mv(void) {
    return rest_mv;
}
