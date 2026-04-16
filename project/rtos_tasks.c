/*
 * tasks.c
 *
 *  Created on: 22 Mar 2026
 *      Author: andre
 */
#include "rtos_tasks.h"

// Periodically trigger ADC sampling for soil moisture updates.
void soilMoisturePollingTask(void *pvParams)
{
	while (1)
	{
		/// Task waits and re-triggers every 2s
		PRINTF("Priority 2 soilMoisturePollingTask starts\r\n");
		ADC_Start(ADC_SE0);
		vTaskDelay(pdMS_TO_TICKS(300));
		PRINTF("Next ADC firing...\r\n");
	}
}

// Wait for critical-dry alerts and send alarm packet over UART.
void alertTask(void *pvParams)
{
	while (1)
	{
		// Task sleeps until ISR fires
		PRINTF("Priority 2 alertTask starts\r\n");
		// Received alert sempahore signal from uartTxTask
		if (xSemaphoreTake(alertSemaphore, portMAX_DELAY) == pdTRUE)
		{
			PRINTF("alertSemaphore taken\r\n");

			// Enter CS: mutex to lock shared variable - send_buffer
			xSemaphoreTake(uartMutex, portMAX_DELAY);
			PRINTF("uartMutex taken\r\n");
			// Only reaches here whens soil is critically dry
			// Notifies ESP, servo control on ESP
			snprintf(send_buffer, MAX_MSG_LEN, "<A,D>\n");
			UART2->C2 |= UART_C2_TE_MASK | UART_C2_TIE_MASK; // Kicks off TX of alert,dry  flag

			// Wait for sending package
			while (UART2->C2 & UART_C2_TIE_MASK)
			{
				vTaskDelay(pdMS_TO_TICKS(2)); // Give up CPU
			}

			xSemaphoreGive(uartMutex);
			PRINTF("uartMutex released\r\n");
			// End of CS
		}
	}
}

// Wait for button interrupt and send manual watering command to ESP32.
void manualWaterTask(void *pvParams)
{
	PRINTF("Priority 4 manualWaterTask starts\r\n");

	while (1)
	{
		// Block and wait for button interrupt
		if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY) == pdTRUE)
		{
			PRINTF("Button pressed! Manual override triggered.\r\n");

			// Software debounce delay (300ms)
			vTaskDelay(pdMS_TO_TICKS(300));
			xQueueReset(buttonSemaphore);

			// Enter critical section
			xSemaphoreTake(uartMutex, portMAX_DELAY);
			PRINTF("uartMutex taken by manualWaterTask\r\n");

			// Send manual override command to ESP32
			snprintf(send_buffer, MAX_MSG_LEN, "<V,1>\n");
			UART2->C2 |= UART_C2_TE_MASK | UART_C2_TIE_MASK;

			// Wait until transmission is complete
			while (UART2->C2 & UART_C2_TIE_MASK)
			{
				vTaskDelay(pdMS_TO_TICKS(2));
			}

			xSemaphoreGive(uartMutex);
			PRINTF("uartMutex released by manualWaterTask\r\n");
		}
	}
}
