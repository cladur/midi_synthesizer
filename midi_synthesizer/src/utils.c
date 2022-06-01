#include "utils.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

#include <stddef.h>

#include "pca9532.h"
#include "oled.h"

#define PCLK_DAC_IN_MHZ         25 //CCLK divided by 4
// TODO: Move it back to main.c and parametrize lut_fill_with_... functions with it
#define WAVE_SAMPLES_COUNT 60

/**
 * @brief Fills array with sine values.
 *
 * @note This function expects the array to hold at least 60 elements. Failing to do so will result in a segmentation fault.
 *
 * @param wave_lut Pointer to the array to fill.
 *
 * @return None
 */
void lut_fill_with_sine(uint32_t *wave_lut) {
    // Array holding first quarter of sine values.
    uint32_t sin_0_to_90_16_samples[16]={\
            0,1045,2079,3090,4067,\
            5000,5877,6691,7431,8090,\
            8660,9135,9510,9781,9945,10000\
    };
    // We write sine values to the wave_lut by transposing sin_0_to_90_16_samples.
    for(int i = 0; i < WAVE_SAMPLES_COUNT; i++)
    {
        if(i<=15)
        {
            wave_lut[i] = (uint32_t)512 + (uint32_t)512*sin_0_to_90_16_samples[i]/(uint32_t)10000;
            if(i==15) {
                wave_lut[i]= 1023;
            }
        }
        else if(i<=30)
        {
            wave_lut[i] = (uint32_t)512 + (uint32_t)512*sin_0_to_90_16_samples[30-i]/(uint32_t)10000;
        }
        else if(i<=45)
        {
            wave_lut[i] = (uint32_t)512 - (uint32_t)512*sin_0_to_90_16_samples[i-30]/(uint32_t)10000;
        }
        else
        {
            wave_lut[i] = (uint32_t)512 - (uint32_t)512*sin_0_to_90_16_samples[60-i]/(uint32_t)10000;
        }
        wave_lut[i] = (wave_lut[i]<<6);
    }

}

/**
 * @brief Fills array with zeroes.
 *
 * @note This function expects the array to hold at least 60 elements. Failing to do so will result in a segmentation fault.
 *
 * @param wave_lut Pointer to the array to fill.
 *
 * @return None
 */
void lut_fill_with_zeroes(uint32_t *wave_lut) {
    for(int i=0;i<WAVE_SAMPLES_COUNT;i++)
    {
        wave_lut[i] = 0;
    }
}

/**
 * @brief Converts integer to string.
 *
 * @param value Integer to be converted to string.
 * @param pBuf  Buffer to store the string.
 * @param len   Length of the buffer.
 * @param base  Base that we want to write the number in (for example 10 or 16).
 *
 * @return None
 */
void int_to_string(int value, uint8_t* pBuf, uint32_t len, uint32_t base) {
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;
    int tmpValue_2 = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (!((pBuf == NULL) || (len < (uint32_t)2) || (base < (uint32_t)2) || (base > (uint32_t)36)))
    {
        // negative value
        if (value < 0)
        {
            tmpValue = -tmpValue;
            tmpValue_2    = -tmpValue_2;
            pos++;
            pBuf[pos] = '-';
        }

        // calculate the required length of the buffer
        do {
            pos++;
            tmpValue /= base;
        } while(tmpValue > 0);

        pBuf[pos] = '\0';

        do {
            pos--;
            pBuf[pos] = pAscii[(uint32_t)tmpValue_2 % base];
            tmpValue_2 /= base;
        } while(tmpValue_2 > 0);
    }
}

/**
 * @brief   Change the frequency of the wave that's playing on the Speaker
 * @note    The calculated value is clock ticks between samples
 * @param   freq    The new frequency of the wave in Hz
 * @return  None
 */
void dac_update_frequency(uint32_t freq) {
    uint32_t tmp = (uint32_t)((uint32_t)PCLK_DAC_IN_MHZ * (uint32_t)1000000) / (uint32_t)(freq * (uint32_t)WAVE_SAMPLES_COUNT);
    DAC_SetDMATimeOut(LPC_DAC, tmp);
}

/**
 * @brief   Checks if the left button is pressed
 * @return  bool    true if left button is pressed, false if it's not
 */
bool button_left_is_pressed(void) {
    return !((GPIO_ReadValue(0) >> 4) & 0x01);
}

/**
 * @brief   Checks if the right button is pressed
 * @return  bool    true if right button is pressed, false if it's not
 */
bool button_right_is_pressed(void) {
    return !((GPIO_ReadValue(1) >> 31) & 0x01);
}

// Functions below trigger MISRA 2012 rule 8.7.
// In order to solve the rule, we should move them to main.c,
// but instead we consciously disapply the rule, because we feel
// like they belong here instead and the place in which they are
// used could change in the future.

/**
 * @brief   This function raises the volume of the speaker by one step.
 *
 * @return  None
 */
void volume_up(void) {
    GPIO_SetValue(0, 1UL<<28);
    Timer0_Wait(1);
    GPIO_SetValue(0, 1UL<<27);
    Timer0_Wait(1);
    GPIO_ClearValue(0, 1UL<<27);
    Timer0_Wait(1);
    GPIO_ClearValue(0, 1UL<<28);
    Timer0_Wait(1);
}

/**
 * @brief   This function decreases the volume of the speaker by one step.
 *
 * @return  None
 */
void volume_down(void) {
    GPIO_ClearValue(0, 1UL<<28);
    Timer0_Wait(1);
    GPIO_SetValue(0, 1UL<<27);
    Timer0_Wait(1);
    GPIO_ClearValue(0, 1UL<<27);
    Timer0_Wait(1);
}

/**
 * @brief   This funtion sets the volume of the speaker to the specified value.
 *
 * @param   volume    The exact value of the volume. Where 0 is the lowest and 15 is the highest.
 *
 * @return  None
 */
void reset_volume(int volume_level) {
    for (int i = 0; i < 15; i++) {
        volume_down();
    }
    for (int i = 0; i < volume_level; i ++) {
        volume_up();
    }
}

/**
 * @brief   Determines which color to use for background and foreground depending on whenever or not
 *          we're using dark mode and if the menu entry is selected.
 *
 * @param   is_dark_mode        Is the dark mode enabled.
 * @param   is_active           Is the menu entry currently selected.
 * @param   foreground_color    Pointer to the variable that will hold the foreground color.
 * @param   background_color    Pointer to the variable that will hold the background color.
 *
 * @return  None
 */
void what_colors_to_use(bool is_dark_mode, bool is_active, int* foreground_color, int* background_color) {
    if ((is_dark_mode ^ is_active) == true) {
        *foreground_color = OLED_COLOR_WHITE;
        *background_color = OLED_COLOR_BLACK;
    } else {
        *foreground_color = OLED_COLOR_BLACK;
        *background_color = OLED_COLOR_WHITE;
    }
}

/**
 * @brief   Draws frequency menu entry on to the screen.
 *
 * @param   is_dark_mode        Is the dark mode enabled.
 * @param   is_active           Is the menu entry currently selected.
 * @param   wave_frequency      New value of wave frequency to be displayed.
 *
 * @return  None
 */
void redraw_frequency(bool is_dark_mode, bool is_active, int wave_frequency) {
    int foreground_color = 0;
    int background_color = 0;
    what_colors_to_use(is_dark_mode, is_active, &foreground_color, &background_color);

    oled_fillRect(0, 0, 100, 9, background_color);
    oled_putString(1,1, (const uint8_t*)"Freq: ", foreground_color, background_color);
    uint8_t wave_frequency_text[10] = {0};
    int_to_string(wave_frequency, wave_frequency_text, 10, 10);
    oled_putString(1 + (6 * 6), 1, wave_frequency_text, foreground_color, background_color);
}

/**
 * @brief   Draws volume menu entry on to the screen.
 *
 * @param   is_dark_mode        Is the dark mode enabled.
 * @param   is_active           Is the menu entry currently selected.
 * @param   volume_level        New value of volume level to be displayed.
 *
 * @return  None
 */
void redraw_volume(bool is_dark_mode, bool is_active, int volume_level) {
    int foreground_color = 0;
    int background_color = 0;
    what_colors_to_use(is_dark_mode, is_active, &foreground_color, &background_color);

    oled_fillRect(0, 10, 100, 18, background_color);
    oled_putString(1, 10, (const uint8_t*)"Vol: ", foreground_color, background_color);
    uint8_t volume_level_text[10] = {0};
    int_to_string(volume_level, volume_level_text, 10, 10);
    oled_putString(1 + (6 * 6), 10, volume_level_text, foreground_color, background_color);
}

/**
 * @brief   Redraws necessary part of the screen.
 *
 * @param   is_dark_mode        Is the dark mode enabled.
 * @param   dark_mode_changed   Determines if we need to redraw the whole screen or only part of it.
 * @param   new_frequency       New value of wave frequency to be displayed.
 * @param   new_volume          New value of volume level to be displayed.
 * @param   active_menu_entry   Currently selected menu entry.
 *
 * @return  None
 */
void refresh_screen(bool is_dark_mode, bool dark_mode_changed, int new_frequency, int new_volume, enum MenuEntry active_menu_entry, enum WhatToRedraw what_to_redraw) {
    if (dark_mode_changed) {
        oled_clearScreen(is_dark_mode ? OLED_COLOR_BLACK : OLED_COLOR_WHITE);
    }
    if (BITWISE_AND(what_to_redraw, REDRAW_FREQUENCY)) {
        redraw_frequency(is_dark_mode, active_menu_entry == MENU_ENTRY_FREQUENCY, new_frequency);
    }
    if (BITWISE_AND(what_to_redraw, REDRAW_VOLUME)) {
        redraw_volume(is_dark_mode, active_menu_entry == MENU_ENTRY_VOLUME, new_volume);
    }
}
