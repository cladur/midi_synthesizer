#ifndef INITS_H
#define INITS_H

#include <stdbool.h>

#include "lpc17xx_uart.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_dac.h"

void init_uart();
void init_i2c();
void init_ssp();
void init_adc();
void init_dac();
void init_amplifier(bool start_muted);
void dac_dma_setup(GPDMA_LLI_Type* DMA_LLI_Struct, GPDMA_Channel_CFG_Type* GPDMACfg, DAC_CONVERTER_CFG_Type* DAC_ConverterConfigStruct, uint32_t* dac_wave_lut, uint32_t dma_size, uint32_t initial_wave_frequency);

#endif
