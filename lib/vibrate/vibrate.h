// The vibration motor on P03. Calls block for the length of the pattern.
// It needs battery power: on the 3.3 V from a USB-serial adapter alone the
// motor may not spin.
#ifndef VIBRATE_H
#define VIBRATE_H

void vibrate_init(void);
void vibrate_ms(int ms);
void vibrate_pulses(int count, int on_ms, int off_ms);

#endif
