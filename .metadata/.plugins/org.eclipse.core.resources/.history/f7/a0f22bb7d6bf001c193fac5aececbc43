#include "utils.h"

#include "pca9532.h"

void lut_fill_with_sine(uint32_t *wave_lut) {
    uint32_t sin_0_to_90_16_samples[16]={\
            0,1045,2079,3090,4067,\
            5000,5877,6691,7431,8090,\
            8660,9135,9510,9781,9945,10000\
    };
    //Prepare DAC sine look up table
    for(uint32_t i = 0; i < WAVE_SAMPLES_COUNT; i++)
    {
        if(i<=15)
        {
            wave_lut[i] = 512 + 512*sin_0_to_90_16_samples[i]/10000;
            if(i==15) wave_lut[i]= 1023;
        }
        else if(i<=30)
        {
            wave_lut[i] = 512 + 512*sin_0_to_90_16_samples[30-i]/10000;
        }
        else if(i<=45)
        {
            wave_lut[i] = 512 - 512*sin_0_to_90_16_samples[i-30]/10000;
        }
        else
        {
            wave_lut[i] = 512 - 512*sin_0_to_90_16_samples[60-i]/10000;
        }
        wave_lut[i] = (wave_lut[i]<<6);
    }

}

void lut_fill_with_zeroes(uint32_t *wave_lut) {
    for(uint32_t i=0;i<NUM_SINE_SAMPLE;i++)
    {
        wave_lut[i] = 0;
    }
}

void int_to_string(int value, uint8_t* pBuf, uint32_t len, uint32_t base) {
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2)
    {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36)
    {
        return;
    }

    // negative value
    if (value < 0)
    {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len)
    {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;

}

void led_update(int led) {
    uint16_t ledOn = 1;

    ledOn = ledOn << (led % 8);

    pca9532_setLeds(ledOn, 0xffff);   
}

/*
 * @brief   Change the frequency of the wave that's playing on the Speaker
 * @param   freq    The new frequency of the wave in Hz
 * @return  None
 */
void dac_update_frequency(uint32_t freq) {
    uint32_t tmp = (PCLK_DAC_IN_MHZ * 1000000) / (freq * NUM_SINE_SAMPLE);
    DAC_SetDMATimeOut(LPC_DAC, tmp);
}

/*
 * @brief   Checks if the left button is pressed
 * @return  bool    true if left button is pressed, false if it's not
 */
bool button_left_is_pressed() {
    !((GPIO_ReadValue(0) >> 4) & 0x01);
}

/*
 * @brief   Checks if the right button is pressed
 * @return  bool    true if right button is pressed, false if it's not
 */
bool button_right_is_pressed() {
    !((GPIO_ReadValue(1) >> 31) & 0x01);
}