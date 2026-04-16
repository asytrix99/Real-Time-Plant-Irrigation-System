// Configure GPIO edge interrupt on PTA4 (SW3).

void initInterrupt()
{
    // Disable interrupts
    NVIC_DisableIRQ(PORTA_IRQn);

    // Set clock gating SCGC5 for PORTA
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    // PTA4 to GPIO
    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_MUX(1);

    // Set pullup resistor
    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_PS_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_PS(1);
    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_PE_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_PE(1);

    // Set as input
    GPIOA->PDDR &= ~(1 << SWITCH_PIN);

    // Configure the interrupt for rising edge
    PORTA->PCR[SWITCH_PIN] &= ~PORT_PCR_IRQC_MASK;
    PORTA->PCR[SWITCH_PIN] |= PORT_PCR_IRQC(0b1010);

    // Set NVIC priority to 192,
    // Lowest priority
    NVIC_SetPriority(PORTA_IRQn, 192);

    // Clear pending interrupts and enable interrupts
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
}

// Button ISR: cycle mode state and clear interrupt status flag.
void PORTA_IRQHandler()
{

    // Check which pin triggered interrupt
    if (PORTA->ISFR & (1 << SWITCH_PIN))
    {

        mode = (mode + 1) % 3; // example logic

        // ISFR must be cleared → otherwise infinite interrupt loop
        PORTA->ISFR |= (1 << SWITCH_PIN); // MUST clear flag
    }

    NVIC_ClearPendingIRQ(PORTA_IRQn);
}