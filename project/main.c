/* Includes */
// Default libraries
#include "globals.h"

#include "adc.h"
#include "led.h"
#include "rtos_tasks.h"
#include "uart.h"

#define BUTTON_PIN 4

char send_buffer[MAX_MSG_LEN];

QueueHandle_t queue;
QueueHandle_t sensorQueue;         // Moisture readings -> UART TX
QueueHandle_t ledQueue;            // LED command
SemaphoreHandle_t uartMutex;       // Protect shared variable send_buffer (accessed by uartTxTask + alertTask)
SemaphoreHandle_t alertSemaphore;  // Binary semaphore for signaling between uartTxTask + alertTask
SemaphoreHandle_t buttonSemaphore; // Semaphore for Button Interrupt

// Configure button GPIO and interrupt routing for manual watering override.
void Button_Init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    PORTA->PCR[BUTTON_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTA->PCR[BUTTON_PIN] |= PORT_PCR_MUX(1);
    PORTA->PCR[BUTTON_PIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTA->PCR[BUTTON_PIN] &= ~PORT_PCR_IRQC_MASK;
    PORTA->PCR[BUTTON_PIN] |= PORT_PCR_IRQC(0b1010);
    GPIOA->PDDR &= ~(1 << BUTTON_PIN);
    NVIC_SetPriority(PORTA_IRQn, 2);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
}

// Button ISR: clear the edge flag and wake the manual water task.
void PORTA_IRQHandler(void)
{
    PORTA->ISFR |= (1 << BUTTON_PIN);
    BaseType_t hpw = pdFALSE;
    xSemaphoreGiveFromISR(buttonSemaphore, &hpw);
    portYIELD_FROM_ISR(hpw);
}

int main(void)
{

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();

#endif

    GPIO_Init();
    PRINTF("GPIO initialized\r\n");

    // Create all RTOS sync primitives before tasks start running.
    queue = xQueueCreate(QLEN, sizeof(TMessage));
    sensorQueue = xQueueCreate(QLEN, sizeof(int)); // hold single moisture value
    ledQueue = xQueueCreate(QLEN, sizeof(int));    // holds single command number
    uartMutex = xSemaphoreCreateMutex();
    alertSemaphore = xSemaphoreCreateBinary();
    buttonSemaphore = xSemaphoreCreateBinary(); // Binary Semaphore for Button

    PRINTF("All queues and semaphores/mutexes created\r\n");

    ADC_Init();
    PRINTF("ADC initialized\r\n");

    initUART2(BAUD_RATE);
    PRINTF("UART initialized\r\n");

    Button_Init();
    PRINTF("Button initialized\r\n");

    // Start producer/consumer tasks for sensing, comms, and LED state control.
    xTaskCreate(soilMoisturePollingTask, "soil_poll", configMINIMAL_STACK_SIZE + 100, NULL, 2, NULL);
    xTaskCreate(alertTask, "alert", configMINIMAL_STACK_SIZE + 100, NULL, 2, NULL);
    xTaskCreate(uartTxTask, "uart_tx", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL);
    xTaskCreate(uartRxTask, "uart_rx", configMINIMAL_STACK_SIZE + 500, NULL, 3, NULL);
    xTaskCreate(ledControlTask, "led_ctrl", configMINIMAL_STACK_SIZE + 100, NULL, 2, NULL);
    xTaskCreate(manualWaterTask, "manual_wtr", configMINIMAL_STACK_SIZE + 100, NULL, 4, NULL);

    PRINTF("All tasks created\r\n");

    // Start the scheduler
    vTaskStartScheduler();

    /* Force the counter to be placed into memory. */
    volatile static int i = 0;
    /* Enter an infinite loop, just incrementing a counter. */
    while (1)
    {
        i++;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile("nop");
    }
    return 0;
}
