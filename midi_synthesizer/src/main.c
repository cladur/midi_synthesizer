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
#include "rgb.h"
#include "pca9532.h"
#include "joystick.h"
#include "eeprom.h"
#include "oled.h"

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

#define VOLUME_INITIAL          10
#define VOLUME_MIN              0
#define VOLUME_MAX              15

#define UART_DEV LPC_UART3

// Forward declarations to comply with MISRA 2012 8.4 rule.
// ....

// -------- VARIABLES FOR PROGRAM STATE --------
// These structs need to be global, if we put them inside main() they will not work.
// MISRA will complain about this (rule 8.9), saying that we should put them inside
// main(), but we can't do that.
static GPDMA_Channel_CFG_Type GPDMACfg = {0};
static GPDMA_LLI_Type DMA_LLI_Struct = {0};
static DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct = {0};

struct EepromData {
    int wave_frequency;
    int volume_level;
};

int main(void) {
    // Lookup table for DAC.
    uint32_t wave_lut[WAVE_SAMPLES_COUNT] = {0};

    // -------- INITIALIZE PERIPHERALS --------
    init_i2c();
    init_ssp();
    init_uart();
    init_adc();
    init_amplifier(); //need to be in that order
    init_dac();

    rotary_init();
    oled_init();
    pca9532_init();
    joystick_init();
    eeprom_init();

    light_init();
    light_enable();
    light_setRange(LIGHT_RANGE_4000);

    lut_fill_with_sine(wave_lut);

    // Read data from EEPROM
    struct EepromData eeprom_data = {0};
    int eeprom_offset = 240;
    int len = eeprom_read((uint8_t*)&eeprom_data, eeprom_offset, sizeof(eeprom_data));
    if ((len != (int)sizeof(eeprom_data)) ||
        (eeprom_data.wave_frequency < WAVE_FREQUENCY_MIN) || (eeprom_data.wave_frequency > WAVE_FREQUENCY_MAX) ||
        (eeprom_data.volume_level < VOLUME_MIN) || (eeprom_data.volume_level > VOLUME_MAX))
    {
        UART_SendString(UART_DEV, (const uint8_t*)"EEPROM: Invalid data, using defaults\r\n");
        eeprom_data.wave_frequency = WAVE_FREQUENCY_INITIAL;
        eeprom_data.volume_level = VOLUME_INITIAL;
    } else {
        UART_SendString(UART_DEV, (const uint8_t*)"EEPROM: Data read succesfully\r\n");
    }
    int wave_frequency = eeprom_data.wave_frequency;
    // Buffer for storing wave frequency value in text form.
    // Used for printing onto the display.
    int volume_level = eeprom_data.volume_level; // TODO: Reset volume level every time to this value

    // -------- SETUP DAC - DMA TRANSFER --------
    dac_dma_setup(&DMA_LLI_Struct, &GPDMACfg, &DAC_ConverterConfigStruct, wave_lut, DMA_SIZE, wave_frequency);

    reset_volume(volume_level);

    // TODO: Better name for these variables?
    int led_counter = 0;
    unsigned int led_index = 0;

    bool is_dark_mode = light_read() < LIGHT_MODE_THRESHOLD;

    // index of currently selected menu option
    enum MenuEntry active_menu_entry = MENU_ENTRY_FREQUENCY;

    // -------- PREPARE DISPLAY --------
    refresh_screen(is_dark_mode, true, wave_frequency, volume_level, active_menu_entry, REDRAW_ALL);

    while (1) {
        bool frequency_changed = false;
        bool volume_changed = false;

        uint8_t joystick_value = joystick_read();
        uint8_t rotary_value = rotary_read();

        enum MenuEntry last_active_menu_entry = active_menu_entry;

        if (BITWISE_AND(joystick_value, JOYSTICK_UP)) {
            active_menu_entry++;
            active_menu_entry = (active_menu_entry) % MENU_ENTRY_COUNT;
        }
        if (BITWISE_AND(joystick_value, JOYSTICK_DOWN)) {
            active_menu_entry--;
            active_menu_entry = (active_menu_entry) % MENU_ENTRY_COUNT;
        }

        if (active_menu_entry != last_active_menu_entry) {
            refresh_screen(is_dark_mode, false, wave_frequency, volume_level, active_menu_entry, REDRAW_ALL);
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
                } else {
                    // If there was no rotary movement, we don't do anything.
                }
                break;
            case MENU_ENTRY_VOLUME:
                if (rotary_value == ROTARY_LEFT) {
                    if (volume_level > VOLUME_MIN) {
                        volume_level -= 1;
                        volume_down();
                    }
                    volume_changed = true;
                } else if (rotary_value == ROTARY_RIGHT) {
                    if (volume_level < VOLUME_MAX) {
                        volume_level += 1;
                        volume_up();
                    }
                    volume_changed = true;
                } else {
                    // If there was no rotary movement, we don't do anything.
                }
                break;
            default:
                break;
        }

        if (frequency_changed) {
            refresh_screen(is_dark_mode, false, wave_frequency, volume_level, active_menu_entry, REDRAW_FREQUENCY);

            dac_update_frequency(wave_frequency);

            eeprom_data.wave_frequency = wave_frequency;
            len = eeprom_write((uint8_t*)&eeprom_data, eeprom_offset, sizeof(eeprom_data));
            if (len != (int)sizeof(eeprom_data)) {
                UART_SendString(UART_DEV, (const uint8_t*)"EEPROM: Failed to write data\r\n");
            }
        }

        if (volume_changed) {
            refresh_screen(is_dark_mode, false, wave_frequency, volume_level, active_menu_entry, REDRAW_VOLUME);

            eeprom_data.volume_level = volume_level;
            len = eeprom_write((uint8_t*)&eeprom_data, eeprom_offset, sizeof(eeprom_data));
            if (len != (int)sizeof(eeprom_data)) {
                UART_SendString(UART_DEV, (const uint8_t*)"EEPROM: Failed to write data\r\n");
            }
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
            refresh_screen(is_dark_mode, true, wave_frequency, volume_level, active_menu_entry, REDRAW_ALL);
        }

        led_counter += 10;

        if (led_counter % (WAVE_FREQUENCY_MAX + 1 - wave_frequency) == 0) {
            led_index++;
            set_leds_cyclic(led_index);
        }

        Timer0_Wait(1);
    }

    return 1;
}
