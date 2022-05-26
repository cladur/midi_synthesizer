#ifndef PCA_LEDS_H
#define PCA_LEDS_H

#include <stdint.h>

void set_leds_cyclic(unsigned int iteration);
void set_leds_wave(uint8_t wave);

#endif
