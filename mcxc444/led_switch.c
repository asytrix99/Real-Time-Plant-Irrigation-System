// Initialize switch input and RGB LED output pins.
void GPIO_Init()
{
    // Enable clock gating for ports A, D, E
    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK |
                   SIM_SCGC5_PORTD_MASK |
                   SIM_SCGC5_PORTE_MASK);

    // Configure switch pin as GPIO (input): SW2 (PTC2)or SW3 (PTA4)
    PORTA->PCR[SWITCH_PIN] = PORT_PCR_MUX(1);
    GPIOA->PDDR &= ~(1 << SWITCH_PIN); // 0 = input, active-low

    // Configure LED pins as GPIO (output)
    PORTD->PCR[GREEN_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[RED_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[BLUE_PIN] = PORT_PCR_MUX(1);

    GPIOD->PDDR |= (1 << GREEN_PIN); // 1 = output
    GPIOE->PDDR |= (1 << RED_PIN) | (1 << BLUE_PIN);
}

// Active-low LED ON helper.
void LED_On(int led)
{
    switch (led)
    {
    case RED:
        GPIOE->PCOR |= (1 << RED_PIN);
        break;
    case GREEN:
        GPIOD->PCOR |= (1 << GREEN_PIN);
        break;
    case BLUE:
        GPIOE->PCOR |= (1 << BLUE_PIN);
        break;
    }
}

// Active-low LED OFF helper.
void LED_Off(int led)
{
    switch (led)
    {
    case RED:
        GPIOE->PSOR |= (1 << RED_PIN);
        break;
    case GREEN:
        GPIOD->PSOR |= (1 << GREEN_PIN);
        break;
    case BLUE:
        GPIOE->PSOR |= (1 << BLUE_PIN);
        break;
    }
}