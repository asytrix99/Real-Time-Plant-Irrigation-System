/*
 * led.c
 *
 *  Created on: 22 Mar 2026
 *      Author: andre
 */

#include "led.h"

// Initialize RGB LED GPIO pins.
void GPIO_Init()
{
    // Enable clock gating for ports A, D, E
    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK |
                   SIM_SCGC5_PORTD_MASK |
                   SIM_SCGC5_PORTE_MASK);

    // Configure LED pins as GPIO (output)
    PORTD->PCR[GREEN_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[RED_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[BLUE_PIN] = PORT_PCR_MUX(1);

    GPIOD->PDDR |= (1 << GREEN_PIN); // 1 = output
    GPIOE->PDDR |= (1 << RED_PIN) | (1 << BLUE_PIN);
}

// Drive LOW → ON
void LED_On(int pin)
{
    switch (pin)
    {
    case RED_PIN:
        GPIOE->PCOR |= (1 << RED_PIN);
        break;
    case GREEN_PIN:
        GPIOD->PCOR |= (1 << GREEN_PIN);
        break;
    case BLUE_PIN:
        GPIOE->PCOR |= (1 << BLUE_PIN);
        break;
    }
}

// Drive HIGH → OFF
void LED_Off(int pin)
{
    switch (pin)
    {
    case RED_PIN:
        GPIOE->PSOR |= (1 << RED_PIN);
        break;
    case GREEN_PIN:
        GPIOD->PSOR |= (1 << GREEN_PIN);
        break;
    case BLUE_PIN:
        GPIOE->PSOR |= (1 << BLUE_PIN);
        break;
    }
}

// LED control task: applies steady color state and optional blink overlay.
void ledControlTask(void *pvParams)
{
    PRINTF("Priority 2 ledControlTask starts\r\n");
    int cmd;
    int currentWaterCmd = LED_OFF;
    int isBlinking = 0;
    int blinkState = 0;

    while (1)
    {
        // Non-blocking receive, check for new commands without waiting
        if (xQueueReceive(ledQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            PRINTF("Received LEDQUEUE, proceeding to contorl LED...\r\n");
            switch (cmd)
            {
            case LED_RED:
                PRINTF("Turn on LED -> low level\r\n");
                currentWaterCmd = LED_RED;
                isBlinking = 0;
                LED_On(RED_PIN);
                LED_Off(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            case LED_YELLOW:
                PRINTF("Turn on LED -> mid level\r\n");
                currentWaterCmd = LED_YELLOW;
                isBlinking = 0;
                LED_On(RED_PIN);
                LED_On(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            case LED_GREEN:
                PRINTF("Turn on LED -> high level\r\n");
                currentWaterCmd = LED_GREEN;
                isBlinking = 0;
                LED_Off(RED_PIN);
                LED_On(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            case LED_BLINK:
                isBlinking = 1;
                break;
            case LED_NOBLINK:
                isBlinking = 0;
                break;
            case LED_OFF:
                currentWaterCmd = LED_OFF;
                isBlinking = 0;
                LED_Off(RED_PIN);
                LED_Off(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            }
        }

        // If blinking, toggle every 100ms using the timeout pdMS_TO_TICKS above
        // Runs regardless whether ledQueue is received
        if (isBlinking)
        {
            if (blinkState == 0)
            {
                LED_Off(RED_PIN);
                LED_Off(GREEN_PIN);
                LED_Off(BLUE_PIN);
                blinkState = 1;
            }
            else
            {
                // Restore water colour
                switch (currentWaterCmd)
                {
                case LED_RED:
                    LED_On(RED_PIN);
                    LED_Off(GREEN_PIN);
                    LED_Off(BLUE_PIN);
                    break;
                case LED_YELLOW:
                    LED_On(RED_PIN);
                    LED_On(GREEN_PIN);
                    LED_Off(BLUE_PIN);
                    break;
                case LED_GREEN:
                    LED_Off(RED_PIN);
                    LED_On(GREEN_PIN);
                    LED_Off(BLUE_PIN);
                    break;
                default:
                    LED_Off(RED_PIN);
                    LED_Off(GREEN_PIN);
                    LED_Off(BLUE_PIN);
                    break;
                }
                blinkState = 0;
            }
        }
        else
        {
            // Not blinking — just hold steady colour
            switch (currentWaterCmd)
            {
            case LED_RED:
                LED_On(RED_PIN);
                LED_Off(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            case LED_YELLOW:
                LED_On(RED_PIN);
                LED_On(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            case LED_GREEN:
                LED_Off(RED_PIN);
                LED_On(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            case LED_OFF:
                LED_Off(RED_PIN);
                LED_Off(GREEN_PIN);
                LED_Off(BLUE_PIN);
                break;
            }
        }
    }
}