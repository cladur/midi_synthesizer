#include "pca_leds.h"

#include "pca9532.h"

static uint16_t ledOn = 0;
static uint16_t ledOff = (uint16_t)0xFFFF;

/**
 * @brief Set led states based on current iteration to create cyclicaly moving single led pattern
 *
 * @param iteration
 */
void set_leds_cyclic(unsigned int iteration) {
    ledOff = ledOn; // turn off leds from prevoius `iteration
    uint16_t led_index = (uint16_t)iteration % (uint16_t)8;
    uint16_t direction = (uint16_t)(((iteration % 16U) < 8U) ? 1U : 0U);
    // green leds
    uint16_t high = ((uint16_t)1 << (uint16_t)(direction ? ((15U) - (led_index)) : (led_index + 8U))) & (uint16_t)0xFF00;
    // red leds
    uint16_t low = ((uint16_t)1 << (uint16_t)(direction ? (led_index) : (7U - led_index))) & (uint16_t)0xFF;
    ledOn = low + high;
    pca9532_setLeds(ledOn, ledOff);
}

/**
 * @brief fill less significant bits with ones
 *
 * @param wave_height number of bits to fill
 * @return uint8_t
 */
static uint8_t wave_to_led(uint8_t wave_height) {
    uint8_t led_wave = 0;
    for (uint8_t i = 0; i < wave_height; i++) {
        led_wave++;
        led_wave <<= 1;
    }
    return led_wave;
}

/**
 * @brief Set red leds to represent current wave and green leds to represent previous wave
 *
 * @param wave current wave value
 */
void set_leds_wave(uint8_t wave) {
    ledOff = ledOn; // turn off leds from previous iteration
    uint8_t wave_height = wave / 32U;
    ledOn <<= 8; // move wave from previous iteration to green leds
    ledOn &= (uint16_t)0xFF00; // mask just to be sure
    ledOn += wave_to_led(wave_height);
    pca9532_setLeds(ledOn, ledOff);
}
