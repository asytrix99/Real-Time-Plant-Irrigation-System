/* Includes */
// Default libraries
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
// Our libraries
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Sample main() */
// Bring up LEDs/interrupts, create demo tasks, then start scheduler.
int main(void)
{

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();

    initLEDs();

    offLED(RED);
    offLED(GREEN);
    offLED(BLUE);

    initIRQ();
#endif

    PRINTF("Semaphore from ISR Demo\r\n");

    // Set the blink flag to 0
    sema = xSemaphoreCreateBinary();

    // Create the blinky task
    xTaskCreate(blinkLEDTask, "blink_led",
                configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);

    // Create the hello task
    xTaskCreate(helloTask, "hello",
                configMINIMAL_STACK_SIZE + 100, NULL, 1, NULL);

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