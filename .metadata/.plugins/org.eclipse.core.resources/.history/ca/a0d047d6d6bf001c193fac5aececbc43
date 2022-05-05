#ifndef INITS_H
#define INITS_H

#include "lpc17xx_uart.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"

void init_uart();
void init_i2c();
void init_ssp();
void init_adc();
void init_dac();
void gpdma_setup(GPDMA_LLI_Type* DMA_LLI_Struct, GPDMA_Channel_CFG_Type* GPDMACfg, uint32_t* dac_wave_lut, uint32_t dma_size);

#endif