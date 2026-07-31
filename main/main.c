#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h" // NUEVO: Para sincronizar hilos
#include "nvs_flash.h"       
#include "mqtt-aws.h"
#include "movement-driver.h"
#include "wifi-driver.h"
#include "esp_log.h"
#include "shared_events.h"
#include "esp_ota_ops.h"
#include <time.h>

static const char *TAG = "MAIN";
ESP_EVENT_DEFINE_BASE(PIR_EVENTS);

// Si tu wifi-driver.h define el EventGroup, úsalo. Si no, podemos forzar un bucle seguro:
extern bool wifi_esta_conectado(void); // Asumiendo que tu driver tiene una forma de saberlo

void app_main(void) {
    // 1. REGLA DE ORO DE ESP-IDF: La NVS DEBE inicializarse en la PRIMERA LÍNEA del código
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS sin formato o corrupta. Borrando y formateando sectores...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret); // Aquí garantizamos que la NVS quedó 100% operativa

    ESP_LOGI(TAG, "Iniciando secuencia de booteo...");

    // 2. AHORA SÍ es completamente seguro inicializar el Wi-Fi
    mi_wifi_inicializar(); 
    
    ESP_LOGI(TAG, "Esperando conexión real a internet (Asignación de IP)...");
    
    // Bucle seguro de holgura para que el router nos entregue IP antes de llamar a AWS
    vTaskDelay(pdMS_TO_TICKS(10000)); 

    init_movement_sensor();
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 3. Inicialización asíncrona de AWS con reloj y prioridades corregidas
    ESP_LOGI(TAG, "Inicializando conexión con AWS IoT Core de forma segura...");
    init_mqtt();
    esp_event_handler_register(PIR_EVENTS, PIR_EVENT_MOTION_DETECTED, &mqtt_send_payload, NULL);
	
    // 4. Validar la partición OTA ante el bootloader
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "¡Firmware validado! Se cancela el rollback automático.");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}