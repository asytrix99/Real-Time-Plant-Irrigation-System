// Example flow:
/*  ESP32 sends data
    ↓
    UART receives 1 byte
    ↓
    RDRF interrupt triggers
    ↓
    ISR reads UART->D
    ↓
    build message in buffer
    ↓
    on '\n' :
    ↓
    send to queue
    ↓
    task processes message
*/

// UART ISR: interrupt-driven TX plus newline-terminated RX queue handoff.
void UART2_FLEXIO_IRQHandler(void)
{
    // Send and receive pointers
    static int recv_ptr = 0, send_ptr = 0;
    char rx_data;
    char recv_buffer[MAX_MSG_LEN];

    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    // Checking if send_buffer empty with S1 & Empty Mask
    // Start filling up UART->D with data
    if (UART2->S1 & UART_S1_TDRE_MASK)
    {
        // Once reached end of send_buffer,
        // Stop transmitting, reset send_ptr
        if (send_buffer[send_ptr] == '\0')
        {
            send_ptr = 0;

            // Disable the transmit interrupt
            UART2->C2 &= ~UART_C2_TIE_MASK;

            // Disable the transmitter
            UART2->C2 &= ~UART_C2_TE_MASK;
        }
        // Fill up UART2->D register with data, increment send_ptr
        else
        {
            UART2->D = send_buffer[send_ptr++];
        }
    }

    // Checking if send_buffer full with S1 and Full Mask
    // Start emptying UART2->D into recv_buffer
    if (UART2->S1 & UART_S1_RDRF_MASK)
    {
        TMessage msg;
        rx_data = UART2->D;
        recv_buffer[recv_ptr++] = rx_data;
        // One completed copying data into recv_buffer
        if (rx_data == '\n')
        {
            // Copy over the string
            BaseType_t hpw;
            recv_buffer[recv_ptr] = '\0';
            strncpy(msg.message, recv_buffer, MAX_MSG_LEN);

            // Release CPU voluntarily
            xQueueSendFromISR(queue, (void *)&msg, &hpw);
            portYIELD_FROM_ISR(hpw);

            // Reset recv_ptr
            recv_ptr = 0;
        }
    }
}