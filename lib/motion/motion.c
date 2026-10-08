#include "motion/motion.h"
#include "board.h"
#include "clock.h"

// hal_systick() counts 625 us BLE slots: 350 ms = 560 ticks.
#define QUIET_TICKS 560

static int level;
static uint32_t edges, last_edge;
static bool in_burst;

void motion_init(void) {
    hal_gpio_pin_init(MOTION, GPIO_INPUT);
    hal_gpio_pull_set(MOTION, GPIO_PULL_UP_S);
    level = hal_gpio_read(MOTION);
}

bool motion_poll(void) {
    uint32_t now = hal_systick();
    int l = hal_gpio_read(MOTION);
    bool started = false;
    if (l != level) {
        level = l;
        edges++;
        if (!in_burst)
            started = true;
        in_burst = true;
        last_edge = now;
    } else if (in_burst && now - last_edge >= QUIET_TICKS) {
        in_burst = false;
    }
    return started;
}

int motion_level(void) {
    return level;
}

uint32_t motion_edges(void) {
    return edges;
}
