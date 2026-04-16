/*
 * adc.c
 *
 *  Created on: 22 Mar 2026
 *      Author: andre
 */

#include "adc.h"

// Configure MCG internal reference clock used by ADC timing.
void setMCGIRClk()
{
    // Clear CLKS
    MCG->C1 &= ~MCG_C1_CLKS_MASK;
    // Set IRCLKEN to enable LIRC
    MCG->C1 |= (MCG_C1_CLKS(0b01)) | (MCG_C1_IRCLKEN_MASK);
    // Choose the 2MHz Clock
    MCG->C2 |= MCG_C2_IRCS_MASK;
    // Set FRCDIV and LIRC_DIV2
    MCG->SC &= ~MCG_SC_FCRDIV_MASK;
    MCG->SC |= MCG_SC_FCRDIV(0b0);
    MCG->MC &= ~MCG_MC_LIRC_DIV2_MASK;
    MCG->MC |= MCG_MC_LIRC_DIV2(0b1);
}

// Initialize ADC0 for soil moisture sampling on ADC_SE0.
void ADC_Init()
{
    NVIC_DisableIRQ(ADC0_IRQn);

    // Enable clock gating to relevant configurations
    setMCGIRClk();

    // Enable clock gating to ADC0
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    // Accessing PTE20, turn on clock to PORTE on SIM_SCGC5
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Set PTE20 (ADC0_SE0) to ADC
    PORTE->PCR[ADC_SE0_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[ADC_SE0_PIN] |= PORT_PCR_MUX(0);

    /* Configure the ADC */

    // Enable the ADC interrupt
    ADC0->SC1[0] |= ADC_SC1_AIEN_MASK;

    // Select single-ended ADC
    // Disable for now
    ADC0->SC1[0] &= ~ADC_SC1_DIFF_MASK;
    ADC0->SC1[0] |= ADC_SC1_DIFF(0b0);

    // Set 12-bit resolution
    ADC0->CFG1 &= ~ADC_CFG1_MODE_MASK;
    ADC0->CFG1 |= ADC_CFG1_MODE(0b01);

    // Set to 1 for long sample time -> spend more time more accurate conversion
    ADC0->CFG1 &= ~ADC_CFG1_ADLSMP_MASK;
    ADC0->CFG1 |= ADC_CFG1_ADLSMP(0b1);

    // Selects input clock source to generate internal clock, ADCK
    //    ADC0->CFG1 &= ~ADC_CFG1_ADICLK_MASK;
    //    ADC0->CFG1 |= ADC_CFG1_ADICLK(0b01);

    // Set software trigger
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;

    // Configure alternate voltage reference
    // MCXC uses alt volt ref by default
    ADC0->SC2 &= ~ADC_SC2_REFSEL_MASK;
    ADC0->SC2 |= ADC_SC2_REFSEL(0b01);

    // Use averaging (change if needed)
    ADC0->SC3 &= ~ADC_SC3_AVGE_MASK;
    ADC0->SC3 |= ADC_SC3_AVGE(0b0);

    // Enable 32 sampling average (adjustable)
    ADC0->SC3 &= ~ADC_SC3_AVGS_MASK;
    ADC0->SC3 |= ADC_SC3_AVGS(0b11);

    // Disable continuous mode
    ADC0->SC3 &= ~ADC_SC3_ADCO_MASK;
    ADC0->SC3 |= ADC_SC3_ADCO(0);

    NVIC_SetPriority(ADC0_IRQn, 1);
    NVIC_EnableIRQ(ADC0_IRQn);
}

// Trigger one conversion on the selected ADC channel.
void ADC_Start(int channel)
{
    // Mask and start the channel
    ADC0->SC1[0] &= ~ADC_SC1_ADCH_MASK;
    ADC0->SC1[0] |= ADC_SC1_ADCH(channel);
}

// ADC ISR: capture conversion result and publish it to the sensor queue.
void ADC0_IRQHandler()
{
    // Clear pending IRQ
    NVIC_ClearPendingIRQ(ADC0_IRQn);

    // If conversion complete, saves to result register
    if (ADC0->SC1[0] & ADC_SC1_COCO_MASK)
    {
        // Actual result can be read from ADC0->R[0]
        int moistureVal = ADC0->R[0];
        PRINTF("Moisture value: %d\r\n", moistureVal);

        //    	PRINTF("Moisture level: %d\r\n", moistureVal);
        BaseType_t hpw = pdFALSE;

        // Once done, start the next conversion
        //    	ADC_Start(ADC_SE0);
        //    	PRINTF("ADC started in IRQ HANDLER/r/n");

        // Yield CPU voluntarily after done
        xQueueSendFromISR(sensorQueue, &moistureVal, &hpw);
        //    	PRINTF("sensorQueue sent from IRQ HANDLER/r/n");
        portYIELD_FROM_ISR(hpw);
        //    	PRINTF("Yielded from IRQ HANDLER/r/n");
    }
}
