// Configure TPM0 channels for RGB LED PWM output.
void PWM_Init()
{
    // Enable clocks
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK;

    // Set pins to TPM (PWM mode)
    PORTE->PCR[RED_PIN] = PORT_PCR_MUX(3);
    PORTD->PCR[GREEN_PIN] = PORT_PCR_MUX(4);
    PORTE->PCR[BLUE_PIN] = PORT_PCR_MUX(3);

    // Select clock source
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(3);

    TPM0->SC = 0;

    // Prescaler = 128
    TPM0->SC |= TPM_SC_PS(7);

    // Center-aligned PWM
    TPM0->SC |= TPM_SC_CPWMS_MASK;

    TPM0->MOD = 250; // PWM period

    // Low-true PWM (because LED is active-low)
    TPM0->CONTROLS[4].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK;
    TPM0->CONTROLS[5].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK;
    TPM0->CONTROLS[2].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK;
}

// Update selected PWM channel duty cycle from percentage value.
void PWM_SetDuty()
{

    // Convert % to counter value
    int value = (percent * TPM0->MOD) / 100;

    TPM0->CONTROLS[channel].CnV = value;
}