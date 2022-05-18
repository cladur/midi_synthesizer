#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

// TODO: Move it back to main.c and parametrize lut_fill_with_... functions with it
#define WAVE_SAMPLES_COUNT 60

void lut_fill_with_sine(uint32_t *wave_lut);
void lut_fill_with_zeroes(uint32_t *wave_lut);
void int_to_string(int value, uint8_t* pBuf, uint32_t len, uint32_t base);
void led_update(int led);
void dac_update_frequency(uint32_t freq);
bool button_left_is_pressed();
bool button_right_is_pressed();
void volume_up();
void volume_down();

#endif
