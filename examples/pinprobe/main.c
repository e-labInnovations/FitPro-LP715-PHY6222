// Pin probe for the parts not yet mapped. Every second it reads the ADC on
// P14 and P15 in high-resolution mode (0-800 mV), as the stock battery code
// does (battery = P14 x 5.5, P15 x 2), toggles P23 (pulsed by the stock
// battery code) to see whether it gates a divider, and reads P07, P11, P15
// and P18 with the pulls the stock firmware gives them.
// Results go to the screen and the UART log; plug and unplug the charger and
// touch the button while watching.
#include "app.h"
#include "adc.h"
#include "display/jd9850.h"
#include "display/gfx.h"
#include "fonts/FreeMono9pt7b.h"

#define WHITE  RGB565(255, 255, 255)
#define BLACK  RGB565(0, 0, 0)
#define YELLOW RGB565(255, 220, 0)

static const adc_CH_t adc_chs[] = {ADC_CH2P_P14, ADC_CH3N_P15};
static const char *adc_names[] = {"P14", "P15"};
#define N_ADC (sizeof(adc_chs) / sizeof(adc_chs[0]))

static const struct {
    gpio_pin_e pin;
    gpio_pupd_e pull;
    const char *name;
} inputs[] = {
    {GPIO_P07, GPIO_PULL_UP_S, "P07"},   // stock: strong pull-up, read once at boot
    {GPIO_P11, GPIO_PULL_DOWN, "P11"},   // stock: pull-down, touch key interrupt
    {GPIO_P15, GPIO_PULL_DOWN, "P15"},   // stock: pull-down, interrupt, starts charging logic
    {GPIO_P18, GPIO_FLOATING, "P18"},
};
#define N_IN (sizeof(inputs) / sizeof(inputs[0]))

static adc_Cfg_t adc_cfg = {
    .channel = ADC_BIT(ADC_CH2P_P14) | ADC_BIT(ADC_CH3N_P15),
    .is_continue_mode = FALSE,
    .is_differential_mode = 0x00,
    .is_high_resolution = ADC_BIT(ADC_CH2P_P14) | ADC_BIT(ADC_CH3N_P15),   // as stock
};

static uint16_t samples[N_ADC][32];
static uint8_t sample_n;
static volatile uint8_t done_mask;
static int p23;

static void adc_evt(adc_Evt_t *ev) {
    if (ev->type != HAL_ADC_EVT_DATA)
        return;
    for (unsigned i = 0; i < N_ADC; i++) {
        if (ev->ch == adc_chs[i]) {
            sample_n = ev->size > 32 ? 32 : ev->size;
            for (int k = 0; k < sample_n; k++)
                samples[i][k] = ev->data[k];
            done_mask |= ADC_BIT(ev->ch);
        }
    }
}

static int raw_avg[N_ADC];

// Reads both channels once; mV at the pin into out[], -1 on timeout.
// (In this SDK hal_adc_value_cal() already returns millivolts.)
static void adc_read(int *out) {
    done_mask = 0;   // hal_adc_config_channel() clears the previous config itself
    if (hal_adc_config_channel(adc_cfg, adc_evt) != 0 || hal_adc_start() != 0) {
        for (unsigned i = 0; i < N_ADC; i++)
            out[i] = -1;
        return;
    }
    for (int t = 0; t < 100 && done_mask != adc_cfg.channel; t++)
        WaitMs(1);
    hal_adc_stop();
    for (unsigned i = 0; i < N_ADC; i++) {
        if (!(done_mask & ADC_BIT(adc_chs[i]))) {
            out[i] = -1;
            continue;
        }
        int sum = 0;
        for (int k = 0; k < sample_n; k++)
            sum += samples[i][k] & 0xfff;
        raw_avg[i] = sample_n ? sum / sample_n : 0;
        out[i] = (int)hal_adc_value_cal(adc_chs[i], samples[i], sample_n, 1, 0);
    }
}

static void show(int row, const char *text) {
    gfx_fill_rect(0, row * 16 + 2, LCD_WIDTH, 16, BLACK);
    gfx_set_cursor_no_height(1, row * 16 + 14);
    gfx_print(text);
}

static void fmt_int(char *s, int v) {
    char t[12];
    int n = 0;
    if (v < 0) {
        *s++ = '-';
        v = -v;
    }
    do {
        t[n++] = '0' + v % 10;
        v /= 10;
    } while (v);
    while (n)
        *s++ = t[--n];
    *s = 0;
}

void app_init(void) {
    lcd_init();
    gfx_init(LCD_WIDTH, LCD_HEIGHT, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(WHITE);

    for (unsigned i = 0; i < N_IN; i++) {
        hal_gpio_pin_init(inputs[i].pin, GPIO_INPUT);
        hal_gpio_pull_set(inputs[i].pin, inputs[i].pull);
    }
    hal_gpio_pin_init(GPIO_P23, GPIO_OUTPUT);
    hal_gpio_write(GPIO_P23, 0);
    hal_adc_init();
    LOG("pin probe: ADC P14 P15 high-res (mV), P23 toggled each second, inputs P07 P11 P15 P18");
}

void app_update(void) {
    int mv[N_ADC];
    char line[16];

    p23 ^= 1;
    hal_gpio_write(GPIO_P23, p23);
    WaitMs(20);
    adc_read(mv);

    int lvl[N_IN];
    for (unsigned i = 0; i < N_IN; i++)
        lvl[i] = hal_gpio_read(inputs[i].pin);

    LOG("P23=%d  P14=%dmV (raw %d, x5.5=%d)  P15=%dmV (raw %d)  P07=%d P11=%d P15=%d P18=%d", p23,
        mv[0], raw_avg[0], mv[0] * 11 / 2, mv[1], raw_avg[1], lvl[0], lvl[1], lvl[2], lvl[3]);

    gfx_set_text_color(YELLOW);
    show(0, p23 ? "P23 = 1" : "P23 = 0");
    gfx_set_text_color(WHITE);
    for (unsigned i = 0; i < N_ADC; i++) {
        line[0] = 0;
        for (int k = 0; adc_names[i][k]; k++)
            line[k] = adc_names[i][k], line[k + 1] = 0;
        int n = 3;
        line[n++] = ' ';
        fmt_int(line + n, mv[i]);
        show(1 + i, line);
    }
    for (unsigned i = 0; i < N_IN; i++) {
        line[0] = inputs[i].name[0], line[1] = inputs[i].name[1], line[2] = inputs[i].name[2];
        line[3] = ' ', line[4] = ' ', line[5] = '0' + lvl[i], line[6] = 0;
        show(4 + i, line);
    }
    WaitMs(1000);
}
