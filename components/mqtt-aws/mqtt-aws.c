#include <stdio.h>
#include "mqtt-aws.h"
#include <string.h>
#include <time.h>
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"  
#include "shared_events.h"

void init_mqtt(void);

static const char *TAG = "AWS_MQTT";

// Configuración de Identidad y Tópicos alineados con Terraform
#define AWS_THING_NAME     "esp32-sensor-01" 
#define TOPIC_TELEMETRIA   "sensors/motion/telemetry"
#define TOPIC_AWS_JOBS     "$aws/things/" AWS_THING_NAME "/jobs/notify"
static esp_mqtt_client_handle_t s_mqtt_client = NULL;


// Recuperación binaria a través de símbolos del enlazador (Nombres base sin prefijo de carpetas)
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");

extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[]   asm("_binary_certificate_pem_crt_end");

extern const uint8_t private_pem_key_start[]   asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[]     asm("_binary_private_pem_key_end");


void mqtt_send_payload(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
	// If we haven't connected to AWS yet, don't try to send a message
	 if (s_mqtt_client == NULL) {
	     ESP_LOGE(TAG, "Cannot publish payload: Not connected to AWS IoT yet.");
	     return;
	 }

	 char payload[256];
	 time_t currentTime;
	 
	 // 1. Grab the current calendar time
	 time(&currentTime);
	 char *time_string = ctime(&currentTime);
	 
	 // 2. STRIP THE NEWLINE
	 size_t len = strlen(time_string);
	 if (len > 0 && time_string[len - 1] == '\n') {
	     time_string[len - 1] = '\0';
	 }
	 
	 // 3. Build the clean JSON payload
	 snprintf(payload, sizeof(payload), 
	          "{\"device_id\":\"%s\",\"movement\":\"detected\",\"time\":\"%s\",\"location\":\"Home\"}", 
	          AWS_THING_NAME, time_string); 
	 
	 // 4. Publish it using our saved client pointer
	 esp_mqtt_client_publish(s_mqtt_client, TOPIC_TELEMETRIA, payload, (int)strlen(payload), 1, 0);
	 ESP_LOGI(TAG, "Payload sent successfully to AWS!");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_CONNECTED: {
		    ESP_LOGI(TAG, "Successfully Connected to AWS IoT Core!");
			s_mqtt_client = event->client;

		    // Don't forget to subscribe to your jobs topic too!
		    esp_mqtt_client_subscribe(s_mqtt_client, TOPIC_AWS_JOBS, 1);
		    break;
		} 
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from AWS IoT Core. Retrying...");
			esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
			esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
			esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
            break;
            
        default:
            break;
    }
}

void init_mqtt(void) {
    // Definición estática para evitar corrupción del bloque de memoria en la pila
    static esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(esp_mqtt_client_config_t));

    // 1. Endpoint directo de AWS IoT Core corregido
    mqtt_cfg.broker.address.uri = "mqtts://a264fwcrxua8s7-ats.iot.us-east-1.amazonaws.com";
	
    // 2. Configuración de Amazon Root CA usando el string nativo (Evita errores -0x002C / -0x2180)
 
	// 2. AWS Server Verification (The Root CA)
	mqtt_cfg.broker.verification.certificate = (const char *)aws_root_ca_pem_start;
	mqtt_cfg.broker.verification.certificate_len = (size_t)(aws_root_ca_pem_end - aws_root_ca_pem_start);

    // 3. Configuración de identidad y certificado del dispositivo con longitudes explícitas
    mqtt_cfg.credentials.client_id = AWS_THING_NAME;
    mqtt_cfg.credentials.authentication.certificate = (const char *)certificate_pem_crt_start;
    mqtt_cfg.credentials.authentication.certificate_len = (size_t)(certificate_pem_crt_end - certificate_pem_crt_start);
    
    // 4. Configuración de la Clave Privada RSA del dispositivo con longitudes explícitas
    mqtt_cfg.credentials.authentication.key = (const char *)private_pem_key_start;
    mqtt_cfg.credentials.authentication.key_len = (size_t)(private_pem_key_end - private_pem_key_start);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client != NULL) {
        esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(client);
        ESP_LOGI(TAG, "MQTT Client initialized successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to initialize MQTT structure");
    }
}