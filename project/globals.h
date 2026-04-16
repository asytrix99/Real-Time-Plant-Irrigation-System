/*
 * globals.h
 *
 *  Created on: 22 Mar 2026
 *      Author: andre
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <stdio.h>
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "board.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

/* MESSAGE DEFINES */
// define buffer length
#define MAX_MSG_LEN 256
extern char send_buffer[MAX_MSG_LEN];

extern int DRY_TH;

// define TMessage object
typedef struct tm
{
    char message[MAX_MSG_LEN];
} TMessage;

// define number of TMessage objects in a queue
#define QLEN 5

/* LED DEFINES */
#define LED_RED 0    // low water level
#define LED_YELLOW 1 // mid water level (combine RED + GREEN = YELLOW)
#define LED_GREEN 2  // high water level -> ok
#define LED_BLINK 3  // LDR below threshold
#define LED_OFF 4
#define LED_NOBLINK 5

#define RED_PIN 31  // PTE31
#define GREEN_PIN 5 // PTD5
#define BLUE_PIN 29 // PTE29

/* UART DEFINES */
// defining baud rate, tx, rx, uart pins, uart priority
#define BAUD_RATE 9600
#define UART_TX_PTE22 22
#define UART_RX_PTE23 23
#define UART2_INT_PRIO 128

// defining threhsholds (to be tuned)
#define WL_TH 10
#define LDR_LOW_TH 150

/* UART DEFINES */
// Configure ADC_SE0 on PTE20
#define ADC_SE0 0
#define ADC_SE0_PIN 20

extern QueueHandle_t queue;
extern QueueHandle_t sensorQueue;
extern QueueHandle_t ledQueue;
extern SemaphoreHandle_t alertSemaphore;
extern SemaphoreHandle_t uartMutex;

#endif /* GLOBALS_H_ */
