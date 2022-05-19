#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"

#include "rotary.h"
#include "light.h"
#include "oled.h"
#include "rgb.h"
#include "pca9532.h"
#include "joystick.h"

#include "inits.h"
#include "utils.h"
#include "pca_leds.h"
#include <stdbool.h>
#include <stdint.h>

#define DMA_SIZE                60
#define WAVE_FREQUENCY_INITIAL  440
#define WAVE_FREQUENCY_MIN      10
#define WAVE_FREQUENCY_MAX      800
#define LIGHT_MODE_THRESHOLD    200

#define VOLUME_MIN              0
#define VOLUME_MAX              15

// -------- VARIABLES FOR PROGRAM STATE --------
uint16_t wave_frequency = WAVE_FREQUENCY_INITIAL;
// Buffer for storing wave frequency value in text form.
// Used for printing onto the display.
uint8_t wave_frequency_text[10] = {0};
uint8_t volume_level = 10; // TODO: Reset volume level every time to this value
uint8_t volume_level_text[10] = {0};
// Lookup table for DAC.
uint32_t wave_lut[WAVE_SAMPLES_COUNT] = {0};

// These structs need to be valid for the entire duration of the program
// We can't just use them and let them be freed when they get out of scope
GPDMA_Channel_CFG_Type GPDMACfg;
GPDMA_LLI_Type DMA_LLI_Struct;
DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;

enum MenuEntries {
    MENU_ENTRY_FREQUENCY,
    MENU_ENTRY_VOLUME,
    MENU_ENTRY_COUNT,
};

void redraw_frequency(bool is_dark_mode, bool is_active) {
    int foreground_color, background_color;
    if (is_dark_mode ^ is_active) {
        foreground_color = OLED_COLOR_WHITE;
        background_color = OLED_COLOR_BLACK;
    } else {
        foreground_color = OLED_COLOR_BLACK;
        background_color = OLED_COLOR_WHITE;
    }
    oled_fillRect(0, 0, 100, 9, background_color);
    oled_putString(1,1, (uint8_t*)"Freq: ", foreground_color, background_color);
    int_to_string(wave_frequency, wave_frequency_text, 10, 10);
    oled_putString((1+6*6),1, wave_frequency_text, foreground_color, background_color);
}

void redraw_volume(bool is_dark_mode, bool is_active) {
    int foreground_color, background_color;
    if (is_dark_mode ^ is_active) {
        foreground_color = OLED_COLOR_WHITE;
        background_color = OLED_COLOR_BLACK;
    } else {
        foreground_color = OLED_COLOR_BLACK;
        background_color = OLED_COLOR_WHITE;
    }
    oled_fillRect(0, 10, 100, 18, background_color);
    oled_putString(1, 10, (uint8_t*)"Vol: ", foreground_color, background_color);
    int_to_string(volume_level, volume_level_text, 10, 10);
    oled_putString((1+6*6),10, volume_level_text, foreground_color, background_color);
}

enum WhatToRedraw {
    REDRAW_FREQUENCY = 0x01,
    REDRAW_VOLUME    = 0x02,
    REDRAW_ALL       = 0x03,
};

void refresh_screen(bool is_dark_mode, bool dark_mode_changed, uint8_t active_menu_entry, enum WhatToRedraw what_to_redraw) {
    // TODO: Don't clear the screen if light / dark mode didn't change
    if (dark_mode_changed) {
        oled_clearScreen(is_dark_mode ? OLED_COLOR_BLACK : OLED_COLOR_WHITE);
    }
    if (what_to_redraw & REDRAW_FREQUENCY) {
        redraw_frequency(is_dark_mode, active_menu_entry == MENU_ENTRY_FREQUENCY);
    }
    if (what_to_redraw & REDRAW_VOLUME) {
        redraw_volume(is_dark_mode, active_menu_entry == MENU_ENTRY_VOLUME);
    }
}

int main() {
    // -------- INITIALIZE PERIPHERALS --------
    init_i2c();
    // init_uart();
    init_ssp();
    init_adc();
    init_amplifier(false); //need to be in that order
    init_dac();
    reset_volume(volume_level);


    rotary_init();
    oled_init();
    pca9532_init();
    joystick_init();

    light_init();
    light_enable();
    light_setRange(LIGHT_RANGE_4000);

    lut_fill_with_sine(wave_lut);

    // -------- SETUP DAC - DMA TRANSFER --------
    dac_dma_setup(&GPDMACfg, &DMA_LLI_Struct, &DAC_ConverterConfigStruct, wave_lut, DMA_SIZE, WAVE_FREQUENCY_INITIAL);

    // TODO: Better name for these variables?
    int led_counter = 0;
    int led_index = 0;

    bool is_dark_mode = light_read() < LIGHT_MODE_THRESHOLD;

    // index of currently selected menu option
    uint8_t active_menu_entry = 0;

    // -------- PREPARE DISPLAY --------
    refresh_screen(is_dark_mode, true, active_menu_entry, REDRAW_ALL);

    while (1) {
        bool frequency_changed = false;
        bool volume_changed = false;

        uint8_t joystick_value = joystick_read();
        uint8_t rotary_value = rotary_read();

        uint8_t last_active_menu_entry = active_menu_entry;

        if (joystick_value & JOYSTICK_UP) {
            active_menu_entry = (++active_menu_entry) % MENU_ENTRY_COUNT;
        }
        if (joystick_value & JOYSTICK_DOWN) {
            active_menu_entry = (--active_menu_entry) % MENU_ENTRY_COUNT;
        }

        if (active_menu_entry != last_active_menu_entry) {
            refresh_screen(is_dark_mode, false, active_menu_entry, REDRAW_ALL);
        }

        // TODO maybe move blocks inside cases to functions
        switch (active_menu_entry) {
            case MENU_ENTRY_FREQUENCY:
                if (rotary_value == ROTARY_LEFT) {
                    if (wave_frequency > WAVE_FREQUENCY_MIN) {
                        wave_frequency -= 10;
                    }
                    frequency_changed = true;
                } else if (rotary_value == ROTARY_RIGHT) {
                    if (wave_frequency < WAVE_FREQUENCY_MAX) {
                        wave_frequency += 10;
                    }
                    frequency_changed = true;
                }
                break;
            case MENU_ENTRY_VOLUME:
                if (rotary_value == ROTARY_LEFT) {
                    volume_down();
                    if (volume_level > VOLUME_MIN) {
                        volume_level -= 1;
                    }
                    volume_changed = true;
                } else if (rotary_value == ROTARY_RIGHT) {
                    volume_up();
                    if (volume_level < VOLUME_MAX) {
                        volume_level += 1;
                    }
                    volume_changed = true;
                }
                break;
            default:
                break;
        }

        if (frequency_changed) {
            refresh_screen(is_dark_mode, false, active_menu_entry, REDRAW_FREQUENCY);

            dac_update_frequency(wave_frequency);
        }

        if (volume_changed) {
            refresh_screen(is_dark_mode, false, active_menu_entry, REDRAW_VOLUME);
        }

        if (button_left_is_pressed()) {
            lut_fill_with_zeroes(wave_lut);
        }
        if (button_right_is_pressed()) {
            lut_fill_with_sine(wave_lut);
        }

        bool was_dark_mode = is_dark_mode;

        int light_value = light_read();
        is_dark_mode = light_value < LIGHT_MODE_THRESHOLD;

        if (was_dark_mode != is_dark_mode) {
            refresh_screen(is_dark_mode, true, active_menu_entry, REDRAW_ALL);
        }

        led_counter += 10;

        if (led_counter % (WAVE_FREQUENCY_MAX - wave_frequency) == 0) {
            led_index++;
            set_leds_cyclic(led_index);
        }

        Timer0_Wait(1);
    }

    return 1;
}
