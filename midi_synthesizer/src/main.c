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

// -------- VARIABLES FOR PROGRAM STATE --------
uint16_t wave_frequency = WAVE_FREQUENCY_INITIAL;
// Buffer for storing wave frequency value in text form.
// Used for printing onto the display.
uint8_t wave_frequency_text[10] = {0};
// Lookup table for DAC.
uint32_t wave_lut[WAVE_SAMPLES_COUNT] = {0};

// These structs need to be valid for the entire duration of the program
// We can't just use them and let them be freed when they get out of scope
GPDMA_Channel_CFG_Type GPDMACfg;
GPDMA_LLI_Type DMA_LLI_Struct;
DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;

void refresh_screen(bool is_dark_mode) {
    int background_color = is_dark_mode ? OLED_COLOR_BLACK : OLED_COLOR_WHITE;
    int foreground_color = is_dark_mode ? OLED_COLOR_WHITE : OLED_COLOR_BLACK;
    oled_clearScreen(background_color);
    oled_putString(1,1, (uint8_t*)"Freq: ", foreground_color, background_color);
    int_to_string(wave_frequency, wave_frequency_text, 10, 10);
    oled_putString((1+6*6),1, wave_frequency_text, foreground_color, background_color);
}

int main() {
    // -------- INITIALIZE PERIPHERALS --------
    init_i2c();
    // init_uart();
    init_ssp();
    init_adc();
    init_amplifier(false); //need to be in that order
    init_dac();

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

    int background_color = OLED_COLOR_BLACK;
    int foreground_color = OLED_COLOR_WHITE;
    bool is_dark_mode = light_read() < LIGHT_MODE_THRESHOLD;

    // -------- PREPARE DISPLAY --------
    refresh_screen(is_dark_mode);


    // number of options
    const uint8_t MENU_OPTIONS = 2;
    // index of currently selected menu option
    uint8_t current_option = 0;

    uint8_t rotary_direction = 0;

    while (1) {
        bool frequency_changed = false;
        bool volume_changed = false;

        uint8_t joystick_value = joystick_read();
        uint8_t rotary_value = rotary_read();

        if ((joystick_value & JOYSTICK_UP) != 0) {
            current_option = (++current_option) % MENU_OPTIONS;
        }
        if ((joystick_value & JOYSTICK_DOWN) != 0) {
            current_option = (--current_option) % MENU_OPTIONS;
        }

        switch (rotary_value) {
            case ROTARY_RIGHT: {
                rotary_direction = 1;
                break;
            }
            case ROTARY_LEFT: {
                rotary_direction = -1;
                break;
            }
            default: {
                rotary_direction = 0;
                break;
            }
        }

        // TODO maybe move blocks inside cases to functions
        switch (current_option) {
            case 0:
                wave_frequency += 10 * rotary_direction;
                frequency_changed = rotary_direction;
                break;
            case 1:
                if (rotary_direction == 1) {
                    volume_up();
                    volume_changed = true;
                }
                if (rotary_direction == -1) {
                    volume_down();
                    volume_changed = true;
                }
                break;
            default:
                break;
        }

        if (frequency_changed) {
            int background_color = is_dark_mode ? OLED_COLOR_BLACK : OLED_COLOR_WHITE;
            int foreground_color = is_dark_mode ? OLED_COLOR_WHITE : OLED_COLOR_BLACK;

            // Write wave frequency on the screen
            oled_fillRect((1+6*6),1, 80, 8, background_color);
            int_to_string(wave_frequency, wave_frequency_text, 10, 10);
            oled_putString((1+6*6),1, wave_frequency_text, foreground_color, background_color);

            dac_update_frequency(wave_frequency);
        }

        if (volume_changed) {
            int background_color = is_dark_mode ? OLED_COLOR_BLACK : OLED_COLOR_WHITE;
            int foreground_color = is_dark_mode ? OLED_COLOR_WHITE : OLED_COLOR_BLACK;

            // TODO substitute for volume related values
            //oled_fillRect((1+6*6),2, 80, 8, background_color);
            //int_to_string(wave_frequency, wave_frequency_text, 10, 10);
            //oled_putString((1+6*6),1, wave_frequency_text, foreground_color, background_color);
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
            refresh_screen(is_dark_mode);
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
