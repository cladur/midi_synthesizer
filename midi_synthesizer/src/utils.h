#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>

// These macros are unused in inits.c, but used in utils.c and main.c.
// Because of that there's a MISRA violation inside inits.c, but we overrule it.

// TODO: Move it back to main.c and parametrize lut_fill_with_... functions with it
#define WAVE_SAMPLES_COUNT 60

// Macro for '&' operator that is compliant with MISRA
#define BITWISE_AND(x, y)      (((x) & (y)) == (y))

enum MenuEntry {
    MENU_ENTRY_FREQUENCY,
    MENU_ENTRY_VOLUME,
    MENU_ENTRY_COUNT,
};

enum WhatToRedraw {
    REDRAW_FREQUENCY = 0x01,
    REDRAW_VOLUME    = 0x02,
    REDRAW_ALL       = 0x03,
};

void lut_fill_with_sine(uint32_t *wave_lut);
void lut_fill_with_zeroes(uint32_t *wave_lut);
void int_to_string(int value, uint8_t* pBuf, uint32_t len, uint32_t base);
void dac_update_frequency(uint32_t freq);
bool button_left_is_pressed(void);
bool button_right_is_pressed(void);
void volume_up(void);
void volume_down(void);
void reset_volume(int volume_level);
void what_colors_to_use(bool is_dark_mode, bool is_active, int* foreground_color, int* background_color);
void redraw_frequency(bool is_dark_mode, bool is_active, int wave_frequency);
void redraw_volume(bool is_dark_mode, bool is_active, int volume_level);
void refresh_screen(bool is_dark_mode, bool dark_mode_changed, int new_frequency, int new_volume, enum MenuEntry active_menu_entry, enum WhatToRedraw what_to_redraw);

#endif
