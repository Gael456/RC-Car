#include "uart_link.h"

// Wiring, documented in one place.
#define LINK_UART      UART_NUM_1   // UART port 1 (port 0 is the default console)
#define LINK_TX_PIN    43           // board pin TX1 -> Tiva PB0
#define LINK_RX_PIN    44           // board pin RX0 <- Tiva PB1
#define LINK_BAUD      115200       // must match the Tiva's UART1
#define LINK_RX_BUFFER 256          // driver's receive buffer, in bytes

void uart_link_init(void)
{
    // 1. Describe the serial format in a config struct: 115200 8N1, no flow control.
    const uart_config_t config = {
        .baud_rate  = LINK_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 2. Install the driver (allocates its buffers and interrupt handler).
    ESP_ERROR_CHECK(uart_driver_install(LINK_UART, LINK_RX_BUFFER, 0, 0, NULL, 0));

    // 3. Apply the format from step 1.
    ESP_ERROR_CHECK(uart_param_config(LINK_UART, &config));

    // 4. Route the UART's TX and RX signals to physical pins. No RTS/CTS pins.
    ESP_ERROR_CHECK(uart_set_pin(LINK_UART, LINK_TX_PIN, LINK_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void uart_link_send(int16_t left, int16_t right)
{
    char message[16];

    // Build the text the Tiva's parser expects: "left,right" then a carriage return.
    int length = snprintf(message, sizeof(message), "%d,%d\r", left, right);

    // Hand the bytes to the driver, which transmits them.
    uart_write_bytes(LINK_UART, message, length);
}