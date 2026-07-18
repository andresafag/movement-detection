#include "wifi-driver.h"
#include <stdio.h>
#include "esp_err.h"        
#include "esp_wifi.h"       
#include "esp_event.h"      
#include "esp_netif.h"      
#include "freertos/idf_additions.h"
#include "nvs_flash.h"      
#include "esp_log.h"
#include "esp_sntp.h"
#include "ping/ping_sock.h"
#include "lwip/inet.h"   

static const char *TAG = "WIFI_DRIVER";


void initialize_sntp(void) {
    ESP_LOGI("SNTP", "Initializing SNTP");
    
    // Set operating mode to polling
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    
    // Set the NTP server (pool.ntp.org dynamically finds a close server)
    esp_sntp_setservername(0, "pool.ntp.org");
    
    // Start the SNTP service
    esp_sntp_init();
}

void wait_for_time_sync(void) {
    // Wait until time is synchronized (status will change from 0)
    int retry = 0;
    const int retry_count = 15;
    
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    // Set your local time zone (e.g., EST/EDT for New York)
    // You can find your specific TZ string online
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();
}

void print_current_time(void) {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("TIME", "The current date/time is: %s", strftime_buf);
}

static void cmd_ping_on_ping_end(esp_ping_handle_t hdl, void *args) {
    uint32_t transmitted = 0;
    uint32_t received = 0;
    uint32_t total_time_ms = 0;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));

    if (received > 0) {
        ESP_LOGI(TAG, "Internet check PASSED. Received %lu/%lu responses in %lu ms.", 
                 received, transmitted, total_time_ms);
    } else {
        ESP_LOGE(TAG, "Internet check FAILED. Host unreachable.");
    }
    
    esp_ping_delete_session(hdl);
}

esp_err_t check_internet_connection(void) {
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    
    ip_addr_t target_addr;
    struct in_addr ip4addr;
    inet_aton("8.8.8.8", &ip4addr);
    
    target_addr.u_addr.ip4.addr = ip4addr.s_addr;
    target_addr.type = IPADDR_TYPE_V4;
    ping_config.target_addr = target_addr;
    
    ping_config.count = 3;          
    ping_config.interval_ms = 1000; 
    ping_config.timeout_ms = 2000;  

    esp_ping_callbacks_t cbs = {
        .on_ping_success = NULL,
        .on_ping_timeout = NULL,
        .on_ping_end = cmd_ping_on_ping_end,
        .cb_args = NULL
    };

    esp_ping_handle_t ping;
    esp_err_t err = esp_ping_new_session(&ping_config, &cbs, &ping);
    if (err == ESP_OK) {
        esp_ping_start(ping);
        ESP_LOGI(TAG, "Ping test started...");
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "Failed to initialize ping session");
    return ESP_FAIL;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Extract the disconnection details from event_data
        wifi_event_sta_disconnected_t* disconnect_info = (wifi_event_sta_disconnected_t*) event_data;
        
        // CRITICAL DIAGNOSTIC: Print the reason code
        ESP_LOGE(TAG, "Disconnected from Wi-Fi. Reason Code: %d. Reconnecting...", disconnect_info->reason);
        
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        check_internet_connection();
    }
}

int mi_wifi_inicializar() 
{
    ESP_LOGI(TAG, "Iniciando configuracion de Wi-Fi...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "BRIGETH",          
            .password = "12345678",  
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); 

    ESP_LOGI(TAG, "Wi-Fi iniciado y conectando a %s...", wifi_config.sta.ssid);
	initialize_sntp();
    
    return ESP_OK;
}
