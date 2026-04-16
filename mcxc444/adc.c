// Configure ADC_SE0 on PTE20 and ADC_SE4a on PTE21
#define ADC_SE0 0
#define ADC_SE0_PIN 20
#define ADC_SE4a 4
#define ADC_SE4_PIN 21

// Initialize ADC for alternating reads on two analog input channels.
void ADC_Init()
{
    NVIC_DisableIRQ(ADC0_IRQn);

    // Enable ADC + port
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Set pins to analog mode
    PORTE->PCR[ADC_SE0_PIN] = PORT_PCR_MUX(0);
    PORTE->PCR[ADC_SE4_PIN] = PORT_PCR_MUX(0);

    // Enable interrupt
    ADC0->SC1[0] |= ADC_SC1_AIEN_MASK;

    // 12-bit resolution
    ADC0->CFG1 |= ADC_CFG1_MODE(1);

    // Software trigger
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;

    // Disable averaging & continuous mode
    ADC0->SC3 &= ~ADC_SC3_AVGE_MASK;
    ADC0->SC3 &= ~ADC_SC3_ADCO_MASK;

    NVIC_SetPriority(ADC0_IRQn, 1);
    NVIC_EnableIRQ(ADC0_IRQn);
}

// ADC ISR: store conversion, toggle channel, and trigger next conversion.
void ADC0_IRQHandler()
{
    static int turn = 0; // Persists across interrupts

    if (ADC0->SC1[0] & ADC_SC1_COCO_MASK)
    {

        result[turn] = ADC0->R[0]; // Read result (clears flag)

        turn = 1 - turn; // Toggle channel

        if (turn)
            ADC_Start(ADC_SE0);
        else
            ADC_Start(ADC_SE4a);
    }

    NVIC_ClearPendingIRQ(ADC0_IRQn);
}