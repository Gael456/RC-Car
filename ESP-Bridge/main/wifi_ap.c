/**
 * @file wifi_ap.c
 *
 * @brief Source file for the ESP32 Wi-Fi driver.
 *
 * Connects the ESP32 to an existing Wi-Fi network (station mode) so it can
 * receive movement commands over UDP and forward them to the Tiva over UART.
 * Connection progress is reported through ESP-IDF events: the event handler
 * retries failed connections and signals wifi_init_sta() once the board has
 * an IP address or has given up.
 *
 * @note The network name and password come from wifi_password.h, which is
 *       git-ignored so the credentials are never committed.
 *
 * @note Despite the file name, this currently uses station mode (joins a
 *       router), not access-point mode.
 *
 * @author Gael Esparza Lobatos
 */

#include "wifi_ap.h"
#include "wifi_password.h"

// Network credentials (from wifi_password.h) and retry limit
#define WIFI_SSID      wifi_id
#define WIFI_PASS      wifi_pass
#define MAXIMUM_RETRY  5



// Event group used by the event handler to tell wifi_init_sta() how the
// connection attempt ended (connected or failed).
static EventGroupHandle_t s_wifi_event_group;


// Event group bits:
//  WIFI_CONNECTED_BIT - connected to the router and received an IP address
//  WIFI_FAIL_BIT      - failed to connect after MAXIMUM_RETRY attempts
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Tag printed at the start of every log message from this file
static const char *TAG = "wifi station";

// Number of reconnection attempts made since the last successful connection
static int s_retry_num = 0;


/**
 * @brief Handles Wi-Fi and IP events raised by the ESP-IDF event loop.
 *
 * Called by ESP-IDF (not directly by this code) whenever a registered event
 * occurs:
 *  - Wi-Fi started:      begin connecting to the router.
 *  - Disconnected:       retry up to MAXIMUM_RETRY times, then report failure.
 *  - Got an IP address:  print it, reset the retry count, report success.
 *
 * @param arg        User data passed at registration (unused, NULL).
 * @param event_base Event family: WIFI_EVENT or IP_EVENT.
 * @param event_id   Specific event within that family.
 * @param event_data Event-specific data (the IP address for IP_EVENT_STA_GOT_IP).
 *
 * @return None
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    // Wi-Fi driver has started: make the first connection attempt
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    // Lost the connection or failed to connect: retry, or give up
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    // Router assigned an IP address: the connection is fully up
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_init_sta(void)
{
    // Create the event group the handler uses to report the result
    s_wifi_event_group = xEventGroupCreate();

    // Initialize the TCP/IP stack (gives the board an IP layer)
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop that delivers Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the network interface for station mode
    esp_netif_create_default_wifi_sta();

    // Initialize the Wi-Fi driver with the default settings
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event_handler for all Wi-Fi events and for the got-IP event
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Network name and password to connect to
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    // Select station mode, apply the credentials, and start Wi-Fi
    // (starting raises WIFI_EVENT_STA_START, which begins connecting)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    // Block until the handler sets either the connected or the fail bit
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // Report the outcome
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s",
                 WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s",
                 WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
