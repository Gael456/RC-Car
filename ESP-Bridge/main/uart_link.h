#pragma once            // include guard: safe to include from several files

#include <stdio.h>
#include "driver/uart.h"    // ESP-IDF's UART driver: declares uart_param_config() etc.
#include "esp_err.h"        // ESP_ERROR_CHECK
#include <stdint.h>

// Set up the UART that talks to the Tiva. Call once at start-up.
void uart_link_init(void);

// Send one drive command, e.g. uart_link_send(8000, -8000) sends "8000,-8000\r".
void uart_link_send(int16_t left, int16_t right);