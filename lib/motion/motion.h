// The ball/tilt switch on P18. Call motion_poll() every millisecond or so;
// it returns true once at the start of each burst of switch bounces (one
// shake or step), like the stock firmware's 350 ms grouping.
#ifndef MOTION_H
#define MOTION_H

#include "types.h"

void motion_init(void);
bool motion_poll(void);
int motion_level(void);
uint32_t motion_edges(void);   // raw level changes since init

#endif
