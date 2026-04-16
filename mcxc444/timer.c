// Configure TPM timer period, overflow interrupt, and NVIC priority.

void TIMER_Init()
{

    NVIC_DisableIRQ(IRQn);

    // Enable TPM clock
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;

    // Select MCGIRCLK as TPM clock source
    SIM->SOPT2 &= ~SIM_SOPT2_TPMSRC_MASK;
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(3);

    TPMx->SC = 0; // Stop timer + clear prescaler

    // Enable overflow interrupt + set prescaler
    TPMx->SC |= TPM_SC_TOIE_MASK | TPM_SC_PS(prescaler);

    TPMx->CNT = 0;   // Reset counter
    TPMx->MOD = mod; // Period

    NVIC_SetPriority(IRQn, priority);
    NVIC_EnableIRQ(IRQn);
}

// TPM overflow ISR: advance state counter and clear timer overflow flag.
void TPM0_IRQHandler()
{

    if (TPM0->STATUS & TPM_STATUS_TOF_MASK)
    {

        count = (count + 1) % 6;

        TPM0->CNT = 0;                       // Reset counter
        TPM0->STATUS |= TPM_STATUS_TOF_MASK; // Clear overflow flag
    }

    NVIC_ClearPendingIRQ(TPM0_IRQn);
}