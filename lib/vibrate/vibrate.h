// The vibration motor on P03. vibrate_ms() and vibrate_pulses() block for the
// length of the pattern; in a BLE build use vibrate_start() and call
// vibrate_poll() from app_update() instead. It needs battery power: on the 3.3 V from a USB-serial adapter alone the
// motor may not spin.
#ifndef VIBRATE_H
#define VIBRATE_H

void vibrate_init(void);
void vibrate_ms(int ms);
void vibrate_pulses(int count, int on_ms, int off_ms);

// Non-blocking: starts the motor for ms milliseconds (restarting any buzz in
// progress); vibrate_poll() stops it when the time is up.
void vibrate_start(int ms);
void vibrate_poll(void);

#endif
