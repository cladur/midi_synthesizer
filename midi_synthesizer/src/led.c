#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

#include "rgb.h"

int main(void) {

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  uint8_t btn = 0;

  rgb_init();

  while (1) {
    btn = ((GPIO_ReadValue(0) >> 4) & 0x01);
    if (btn == 0) {
      rgb_setLeds(RGB_RED | RGB_GREEN | RGB_BLUE);
    }
  }
}
