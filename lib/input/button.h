// The touch key on P11. Call button_poll() every 10-50 ms; it returns an
// event when one completes. Thresholds follow the stock firmware.
#ifndef BUTTON_H
#define BUTTON_H

#include "types.h"

typedef enum {
    BUTTON_NONE,
    BUTTON_PRESS,       // released before 1.5 s
    BUTTON_LONG,        // held 1.5 s (fires while still held)
    BUTTON_VERY_LONG,   // held 2.5 s (fires while still held)
} button_event_t;

void button_init(void);
bool button_down(void);
button_event_t button_poll(void);

#endif
