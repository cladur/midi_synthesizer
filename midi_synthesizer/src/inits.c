#include "inits.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_timer.h"

#include "utils.h"

#define UART_DEV LPC_UART3

void init_uart(void) {
    PINSEL_CFG_Type PinCfg;
    UART_CFG_Type uartCfg;

    /* Initialize UART3 pin connect */
    PinCfg.Funcnum = 2;
    PinCfg.Pinnum = 0;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 1;
    PINSEL_ConfigPin(&PinCfg);

    uartCfg.Baud_rate = 115200;
    uartCfg.Databits = UART_DATABIT_8;
    uartCfg.Parity = UART_PARITY_NONE;
    uartCfg.Stopbits = UART_STOPBIT_1;

    UART_Init(UART_DEV, &uartCfg);

    UART_TxCmd(UART_DEV, ENABLE);

}

void init_i2c(void) {
    PINSEL_CFG_Type PinCfg;

    /* Initialize I2C2 pin connect */
    PinCfg.Funcnum = 2;
    PinCfg.Pinnum = 10;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 11;
    PINSEL_ConfigPin(&PinCfg);

    // Initialize I2C peripheral
    I2C_Init(LPC_I2C2, 100000);

    /* Enable I2C1 operation */
    I2C_Cmd(LPC_I2C2, ENABLE);
}


void init_ssp(void) {
    SSP_CFG_Type SSP_ConfigStruct;
    PINSEL_CFG_Type PinCfg;

    /*
     * Initialize SPI pin connect
     * P0.7 - SCK;
     * P0.8 - MISO
     * P0.9 - MOSI
     * P2.2 - SSEL - used as GPIO
     */
    PinCfg.Funcnum = 2;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 7;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 8;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = 9;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Funcnum = 0;
    PinCfg.Portnum = 2;
    PinCfg.Pinnum = 2;
    PINSEL_ConfigPin(&PinCfg);

    SSP_ConfigStructInit(&SSP_ConfigStruct);

    // Initialize SSP peripheral with parameter given in structure above
    SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

    // Enable SSP peripheral
    SSP_Cmd(LPC_SSP1, ENABLE);

}

void init_adc(void) {
    PINSEL_CFG_Type PinCfg;

    /*
     * Init ADC pin connect
     * AD0.0 on P0.23
     */
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 23;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    /* Configuration for ADC :
     * 	Frequency at 0.2Mhz
     *  ADC channel 0, no Interrupt
     */
    ADC_Init(LPC_ADC, 200000);
    ADC_IntConfig(LPC_ADC,ADC_CHANNEL_0,DISABLE);
    ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE);
}

void init_dac(void) {
    /*
     * Init DAC pin connect
     * AOUT on P0.26
     */
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 2;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    DAC_Init(LPC_DAC);
}

void init_amplifier(void) {
    GPIO_SetDir(2, 1UL<<0, 1); //GPIO_28
    GPIO_SetDir(2, 1UL<<1, 1); //GPIO_29

    GPIO_SetDir(0, 1UL<<27, 1); // PIO3_0 GPIO_21
    GPIO_SetDir(0, 1UL<<28, 1); // PIO3_1 GPIO_22
    GPIO_SetDir(2, 1UL<<13, 1); // PIO2_13 GPIO_23
    GPIO_SetDir(0, 1UL<<26, 1); // GPIO_14 DAC OUTPUT

    GPIO_ClearValue(0, 1UL<<27); //LM4811-clk AMP clock
    GPIO_ClearValue(0, 1UL<<28); //LM4811-up/dn AMP digital control signal
    GPIO_ClearValue(2, 1UL<<13); //LM4811-shutdn AMP shutdown control signal

    // if (start_muted) {
    //     // Set volume to the lowest value
    //     for (int i = 0; i < 16; i++) {
    //         volume_down();
    //     }
    // } else {
    //     for (int i = 0; i < 16; i++) {
    //         volume_up();
    //     }
    // }
}

void dac_dma_setup(GPDMA_LLI_Type* DMA_LLI_Struct, GPDMA_Channel_CFG_Type* GPDMACfg, DAC_CONVERTER_CFG_Type* DAC_ConverterConfigStruct, uint32_t* dac_wave_lut, uint32_t dma_size, uint32_t initial_wave_frequency) {
    /* GPDMA block section -------------------------------------------- */
    /* Initialize GPDMA controller */
    GPDMA_Init();

    //Prepare DMA link list item structure
    DMA_LLI_Struct->SrcAddr= dac_wave_lut; // This line will either trigger warning from compiler or MISRA :(
    DMA_LLI_Struct->DstAddr= (uint32_t)&(LPC_DAC->DACR);
    DMA_LLI_Struct->NextLLI= (uint32_t)DMA_LLI_Struct;
    DMA_LLI_Struct->Control= dma_size
                            | (2UL<<18) //source width 32 bit
                            | (2UL<<21) //dest. width 32 bit
                            | (1UL<<26) //source increment
                            ;

    // Setup GPDMA channel --------------------------------
    // channel 0
    GPDMACfg->ChannelNum = 0;
    // Source memory
    GPDMACfg->SrcMemAddr = dac_wave_lut;
    // Destination memory - unused
    GPDMACfg->DstMemAddr = 0;
    // Transfer size
    GPDMACfg->TransferSize = dma_size;
    // Transfer width - unused
    GPDMACfg->TransferWidth = 0;
    // Transfer type
    GPDMACfg->TransferType = GPDMA_TRANSFERTYPE_M2P;
    // Source connection - unused
    GPDMACfg->SrcConn = 0;
    // Destination connection
    GPDMACfg->DstConn = GPDMA_CONN_DAC;
    // Linker List Item - unused
    GPDMACfg->DMALLI = (uint32_t)DMA_LLI_Struct;
    // Setup channel with given parameter
    GPDMA_Setup(GPDMACfg);

    DAC_ConverterConfigStruct->CNT_ENA = SET;
    DAC_ConverterConfigStruct->DMA_ENA = SET;
    dac_update_frequency(initial_wave_frequency);
    DAC_ConfigDAConverterControl(LPC_DAC, DAC_ConverterConfigStruct);

    // Enable GPDMA channel 0
    GPDMA_ChannelCmd(0, ENABLE);
}
