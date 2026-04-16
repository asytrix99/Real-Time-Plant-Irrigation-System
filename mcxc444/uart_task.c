#define QLEN 5
QueueHandle_t queue;

// Queues are initialized
QueueHandle_t uartQueue;
QueueHandle_t ledQueue;

typedef struct tm
{
    char message[MAX_MSG_LEN];
} TMessage;

// Before this, tasks are added to uartQueue or ledQueue...

// Higher-priority parser task: translates UART text messages into LED commands.
static void uartReceiveTask(void *p)
{
    TMessage msg;
    uint8_t cmd;
    while (1)
    {
        if (xQueueReceive(uartQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            if (strncmp(msg.message, "ON", 2) == 0)
            {
                cmd = 1;
                xQueueSend(ledQueue, &cmd, 0);
            }
            else if (strncmp(msg.message, "OFF", 3) == 0)
            {
                cmd = 0;
                xQueueSend(ledQueue, &cmd, 0);
            }
        }
    }
}

// Lower-priority actuator task: applies LED action from parsed commands.
static void ledControlTask(void *p)
{
    uint8_t cmd;
    while (1)
    {
        if (xQueueReceive(ledQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
            if (cmd == 1)
            {
                onLED(RED);
            }
            else
            {
                offLED(RED);
            }
        }
    }
}