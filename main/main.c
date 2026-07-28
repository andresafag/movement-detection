#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"       
#include "mqtt-aws.h"
#include "movement-driver.h"
#include "wifi-driver.h"
#include "esp_log.h"
#include "shared_events.h"
#include "esp_ota_ops.h"

static const char *TAG = "MAIN";
ESP_EVENT_DEFINE_BASE(PIR_EVENTS);

void app_main(void) {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
        
    ESP_LOGI(TAG, "Iniciando secuencia de booteo...");

    mi_wifi_inicializar(); 
    
    ESP_LOGI(TAG, "Esperando conexión a internet...");
    vTaskDelay(pdMS_TO_TICKS(5000)); 

    init_movement_sensor();
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Inicializando conexión con AWS IoT Core...");
    init_mqtt();
	esp_event_handler_register(PIR_EVENTS, PIR_EVENT_MOTION_DETECTED, &mqtt_send_payload, NULL);
	
	esp_ota_mark_app_valid_cancel_rollback();
	ESP_LOGI(TAG, "¡Firmware validado! Se cancela el rollback automático.");
    

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}