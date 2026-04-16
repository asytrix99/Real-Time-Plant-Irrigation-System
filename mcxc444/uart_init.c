// Defining baud rate, tx, rx, uart pins, uart priority
#define BAUD_RATE 9600
#define UART_TX_PTE22 22
#define UART_RX_PTE23 23
#define UART2_INT_PRIO 128

// Define buffer length
#define MAX_MSG_LEN 256
char send_buffer[MAX_MSG_LEN];

// Define TMessage object
typedef struct tm
{
    char message[MAX_MSG_LEN];
} TMessage;

// Define number of TMessage objects in a queue
#define QLEN 5
QueueHandle_t queue;

// Configure UART2 peripheral, pins, baud rate, and RX interrupt.
void initUART2(uint32_t baud_rate)
{
    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    // Enable clock to UART2 and PORTE
    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Ensure Tx and Rx are disabled before configuration
    UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

    // Connect UART pins for PTE22, PTE23
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