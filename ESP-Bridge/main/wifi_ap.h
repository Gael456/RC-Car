#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"




/**
 * @brief Connects the ESP32 to the Wi-Fi network in station mode.
 *
 * Initializes the network interface, event loop and Wi-Fi driver, registers
 * the event handler, applies the network credentials and starts Wi-Fi. Then
 * blocks until the connection succeeds or fails after MAXIMUM_RETRY attempts.
 *
 * @note NVS flash must be initialized before calling this function.
 *
 * @return None
 */
void wifi_init_sta(void);

