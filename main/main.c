#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"       
#include "mqtt-aws.h"
#include "movement-driver.h"
#include "wifi-driver.h"
#include "esp_log.h"
#include "shared_events.h"

static const char *TAG = "MAIN";
ESP_EVENT_DEFINE_BASE(PIR_EVENTS);

void app_main(void) {
    // 1. Inicializar el almacenamiento NVS (Requerido por Wi-Fi y TLS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
        
    ESP_LOGI(TAG, "Iniciando secuencia de booteo...");

    // 2. Inicializar el Wi-Fi primero
    // NOTA: Asegúrate de que dentro de 'mi_wifi_inicializar' NO haya un bucle infinito. 
    // Debe configurar el Wi-Fi y salir.
    mi_wifi_inicializar(); 
    
    // 3. Darle un tiempo prudente al Wi-Fi para que se conecte y obtenga IP legítima
    ESP_LOGI(TAG, "Esperando conexión a internet...");
    vTaskDelay(pdMS_TO_TICKS(5000)); // 5 segundos de espera base

    // 4. Inicializar el sensor de movimiento
    init_movement_sensor();
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 5. Ahora que hay IP validada por tu ping test, iniciamos MQTT de forma segura
    ESP_LOGI(TAG, "Inicializando conexión con AWS IoT Core...");
    init_mqtt();
	esp_event_handler_register(PIR_EVENTS, PIR_EVENT_MOTION_DETECTED, &mqtt_send_payload, NULL);
    
    // 6. El app_main ya cumplió su ciclo de vida de inicialización. 
    // Liberamos su stack quedándonos en un bucle pasivo infinito de bajo consumo.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}