/*
 * uart.c
 *
 *  Created on: 22 Mar 2026
 *      Author: andre
 */

#include "uart.h"
#include "led.h"

int DRY_TH;

// --- UART Hardware & ISR ---
void initUART2(uint32_t baud_rate)
{
    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    // enable clock to UART2 and PORTE
    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Ensure Tx and Rx are disabled before configuration
    UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

    // connect UART pins for PTE22, PTE23
    PORTE->PCR[UART_TX_PTE22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_TX_PTE22] |= PORT_PCR_MUX(4);

    PORTE->PCR[UART_RX_PTE23] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_RX_PTE23] |= PORT_PCR_MUX(4);

    // Set the baud rate
    uint32_t bus_clk = CLOCK_GetBusClkFreq();

    // This version of sbr does integer rounding.
    uint32_t sbr = (bus_clk + (baud_rate * 8)) / (baud_rate * 16);

    // Set SBR. Bits 8 to 12 in BDH, 0-7 in BDL.
    // MUST SET BDH FIRST!
    UART2->BDH &= ~UART_BDH_SBR_MASK;
    UART2->BDH |= ((sbr >> 8) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFF);

    // Disable loop mode
    UART2->C1 &= ~UART_C1_LOOPS_MASK;
    UART2->C1 &= ~UART_C1_RSRC_MASK;

    // Disable parity
    UART2->C1 &= ~UART_C1_PE_MASK;

    // 8-bit mode
    UART2->C1 &= ~UART_C1_M_MASK;

    // Enable RX interrupt
    UART2->C2 |= UART_C2_RIE_MASK;

    // Enable the receiver
    UART2->C2 |= UART_C2_RE_MASK;

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);
}

// UART handler for ESP-MCXC communication
void UART2_FLEXIO_IRQHandler(void)
{
    // Send and receive pointers
    static int recv_ptr = 0, send_ptr = 0;
    char rx_data;
    static char recv_buffer[MAX_MSG_LEN];

    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    // checking if send_buffer empty with S1 & Empty Mask
    // start filling up UART->D with data
    //
    if ((UART2->S1 & UART_S1_TDRE_MASK) && (UART2->C2 & UART_C2_TIE_MASK))
    {
        //    	PRINTF("Ready to send data over on UART\r\n");
        // once reached end of send_buffer,
        // stop transmitting, reset send_ptr
        if (send_buffer[send_ptr] == '\0')
        {
            send_ptr = 0;

            // Disable the transmit interrupt
            UART2->C2 &= ~UART_C2_TIE_MASK;

            // Disable the transmitter
            UART2->C2 &= ~UART_C2_TE_MASK;
        }
        else
        {
            // fill up UART2->D register with data, increment send_ptr
            UART2->D = send_buffer[send_ptr++];
        }
    }

    // checking if send_buffer full with S1 and Full Mask
    // start emptying UART2->D into recv_buffer
    if (UART2->S1 & UART_S1_RDRF_MASK)
    {
        //    	PRINTF("Ready to receive data over on UART\r\n");
        TMessage msg;
        rx_data = UART2->D;
        if (recv_ptr < MAX_MSG_LEN - 1) {
            recv_buffer[recv_ptr++] = rx_data;
        }
        // one completed copying data into recv_buffer
        if (rx_data == '\n')
        {
            PRINTF("Reached end of string of recv_buffer\r\n");
            // Copy over the string
            BaseType_t hpw;
            recv_buffer[recv_ptr] = '\0';
            strncpy(msg.message, recv_buffer, MAX_MSG_LEN);

            // Release CPU voluntarily
            xQueueSendFromISR(queue, &msg, &hpw);
            portYIELD_FROM_ISR(hpw);

            // reset recv_ptr
            recv_ptr = 0;
        }
    }
}

void uartTxTask(void *pvParams)
{
    int moisture;
    DRY_TH = 3800;

    while (1)
    {
        PRINTF("Priority 3 uartTxTask starts\r\n");
        PRINTF("uartTxTask waiting to receive sensorQueue\r\n");

        if (xQueueReceive(sensorQueue, &moisture, portMAX_DELAY) == pdTRUE)
        {
            PRINTF("sensorQueue received by uartTxTask\r\n");

            // enter CS: mutex to lock shared variable - send_buffer
            xSemaphoreTake(uartMutex, portMAX_DELAY);
            PRINTF("uartMutex taken by uartTxTask\r\n");
            // allowing truncation
            snprintf(send_buffer, MAX_MSG_LEN, "<M,%d>\n", moisture);

            UART2->C2 |= UART_C2_TE_MASK | UART_C2_TIE_MASK; // kicks off TX of moisture value
            //wait for sending package
            while(UART2->C2 & UART_C2_TIE_MASK){
            	vTaskDelay(pdMS_TO_TICKS(2)); // give up CPU
            }
            xSemaphoreGive(uartMutex);
            // end of CS
            PRINTF("uartMutex released by uartTxTask\r\n");

            // wake alertTask if critically dry
            if (moisture > DRY_TH)
            {
                // signals alertTask
                PRINTF("moisture is > DRY_TH!\r\n");
                xSemaphoreGive(alertSemaphore);
            }
        }
    }
}

void uartRxTask(void *pvParams)
{
    PRINTF("Priority 3 uartRxTask starts\r\n");
    PRINTF("uartRxTask waiting to receive data from ESP\r\n");
    TMessage msg;

    while (1)
    {
        xQueueReceive(queue, &msg, portMAX_DELAY);
        PRINTF("Queue from ESP received\r\n");
        int cmd;
        int waterLevel, lightLevel;
        char cond;

        if (sscanf(msg.message, "<W,%c>", &cond))
        {
            if (cond == 'R')
            {
                DRY_TH = 4000;
                PRINTF("Weather condition: %c\r\n", cond);
                PRINTF("New DRY_TH value: %d\r\n", DRY_TH);
            }
        }

        if (sscanf(msg.message, "<W,%*c><%d, %d>", &waterLevel, &lightLevel) == 2)
        {
            if (waterLevel < WL_TH)
            {
                PRINTF("Low water level detected!\r\n");
                cmd = LED_RED;
                xSemaphoreTake(uartMutex, portMAX_DELAY);
                snprintf(send_buffer, MAX_MSG_LEN, "<A,LOW>\n");
                UART2->C2 |= UART_C2_TE_MASK | UART_C2_TIE_MASK;
                while(UART2->C2 & UART_C2_TIE_MASK){
                	vTaskDelay(pdMS_TO_TICKS(2));
                }
                xSemaphoreGive(uartMutex);
            }
            //        	} else if (waterLevel < WL_HIGH_TH) {
            //        		cmd = LED_YELLOW;
            //        		PRINTF("Mid water level detected!\r\n");
            else
            {
                cmd = LED_GREEN;
                PRINTF("High water level detected!\r\n");
            }
            xQueueSend(ledQueue, &cmd, 0);

            if (lightLevel > LDR_LOW_TH)
            {
                cmd = LED_BLINK;
                PRINTF("Low light level detected!\r\n");
            }
            else
            {
                cmd = LED_NOBLINK;
                PRINTF("Normal light level...\r\n");
            }
            xQueueSend(ledQueue, &cmd, 0);
        }
    }
}
