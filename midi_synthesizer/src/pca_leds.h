#ifndef PCA_LEDS_H
#define PCA_LEDS_H

#include <stdint.h>

void set_leds_loop(int iteration);
uint8_t wave_to_led(uint8_t wave);
void set_leds_wave(uint8_t wave);

#endif
