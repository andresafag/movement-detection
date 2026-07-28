#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "mqtt_client.h"  
#include "sdkconfig.h"      

static const char *TAG = "AWS_MQTT";
static char topic_jobs[128];

#define AWS_THING_NAME     CONFIG_AWS_THING_NAME 
#define AWS_ENDPOINT		CONFIG_AWS_ENDPOINT
#define TOPIC_TELEMETRIA   "sensors/motion/telemetry"



static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_ota_in_progress = false;
static int s_job_pub_msg_id = -1;

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");

extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[]   asm("_binary_certificate_pem_crt_end");

extern const uint8_t private_pem_key_start[]   asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[]     asm("_binary_private_pem_key_end");

// ====================================================================
// REGULAR FUNCTION: Sends the success report (SUCCEEDED) to AWS IoT
// ====================================================================
static int notificar_estatus_job(const char *job_id, int execution_number) {
    if (s_mqtt_client == NULL) return -1;

    char topic_update[128];
    char payload_status[128];

    // Constuye el tópico dinámico con el JobId asignado por tu Lambda
    snprintf(topic_update, sizeof(topic_update), "$aws/things/%s/jobs/%s/update", AWS_THING_NAME, job_id);

    // Construye el JSON con el estatus "SUCCEEDED"
    snprintf(payload_status, sizeof(payload_status), "{\"status\":\"SUCCEEDED\",\"executionNumber\":%d}", execution_number);

    ESP_LOGI(TAG, "Notificando éxito a AWS en: %s", topic_update);
    
	return esp_mqtt_client_publish(s_mqtt_client, topic_update, payload_status, (int)strlen(payload_status), 1, 0);
	
}

// ====================================================================
// PAYLOAD SENDING: Sends the payload to AWS IoT
// ====================================================================
void mqtt_send_payload(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Cannot publish payload: Not connected to AWS IoT yet.");
        return;
    }

    char payload[256];
    putenv("TZ=COT5");
    tzset(); 
    
    time_t currentTime;
    time(&currentTime);
    char *time_string = ctime(&currentTime);
    
    size_t len = strlen(time_string);
    if (len > 0 && time_string[len - 1] == '\n') {
        time_string[len - 1] = '\0';
    }
    
    snprintf(payload, sizeof(payload), "{\"device_id\":\"%s\",\"movement\":\"detected\",\"time\":\"%s\",\"location\":\"Home\"}", AWS_THING_NAME, time_string); 
    
			 int msg_id = esp_mqtt_client_publish(s_mqtt_client, TOPIC_TELEMETRIA, payload, (int)strlen(payload), 1, 0);

			 // 2. Validar si el ID es válido (>= 0 significa éxito en la API MQTT de ESP-IDF)
			 if (msg_id >= 0) {
			 	ESP_LOGI(TAG, "Payload sent successfully to AWS! Msg ID: %d", msg_id);
			 } else {
			 	ESP_LOGE(TAG, "MQTT Publish failed. Return code: %d", msg_id);
			 }
    
}

static void ejecutar_actualizacion_ota(const char *url_firmware) {
    ESP_LOGI(TAG, "Iniciando descarga OTA desde S3...");

    esp_http_client_config_t config_http = {
        .url = url_firmware,
        .cert_pem = (const char *)aws_root_ca_pem_start, 
        .timeout_ms = 10000,
        .keep_alive_enable = true,
        .buffer_size = 2048,     
        .buffer_size_tx = 2048,
    };

    esp_https_ota_config_t config_ota = {
        .http_config = &config_http,
    };

    ESP_LOGW(TAG, "Installing new firmware. Please don't disconnect the device.");
    esp_err_t ret = esp_https_ota(&config_ota);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "¡Flasheo completado con éxito! Volviendo para reportar estatus...");
    } else {
        ESP_LOGE(TAG, "Error crítico durante la escritura OTA: %s", esp_err_to_name(ret));
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "Conectado a AWS IoT Core de forma segura.");
            s_mqtt_client = event->client;
            esp_mqtt_client_subscribe(s_mqtt_client, topic_jobs, 1);
            break;
        } 
        case MQTT_EVENT_DATA: {
            // GUARD: If an OTA is already running or waiting to restart, ignore new incoming MQTT messages!
            if (s_ota_in_progress) {
                ESP_LOGW(TAG, "Mensaje MQTT ignorado: OTA ya en proceso o pendiente de reinicio.");
                break;
            }

            if (strncmp(event->topic, topic_jobs, event->topic_len) == 0) {
                ESP_LOGI(TAG, "Procesando mensaje entrante de Jobs...");
                
                static char current_job_id[64] = {0};
                int exec_number = 0;
                static char url_buffer[1024] = {0};

                char json_str[event->data_len + 1];
                memcpy(json_str, event->data, event->data_len);
                json_str[event->data_len] = '\0';

                char *ptr_job = strstr(json_str, "\"jobId\":");
                char *ptr_exec = strstr(json_str, "\"executionNumber\":");
                char *ptr_url = strstr(json_str, "\"url\":");

                if (ptr_job && ptr_exec && ptr_url) {
                    if (sscanf(ptr_job, "\"jobId\":\"%63[^\"]\"", current_job_id) == 1 &&
                        sscanf(ptr_exec, "\"executionNumber\":%d", &exec_number) == 1 &&
                        sscanf(ptr_url, "\"url\":\"%1023[^\"]\"", url_buffer) == 1) {

                        // Mark OTA in progress to block duplicate executions immediately
                        s_ota_in_progress = true;

                        ESP_LOGI(TAG, "URL de S3 y Job ID '%s' validados exitosamente.", current_job_id);
                        
                        // 1. Unsubscribe right away so AWS job updates don't hit us
                        esp_mqtt_client_unsubscribe(s_mqtt_client, topic_jobs);

                        // 2. Perform the OTA update
                        ejecutar_actualizacion_ota(url_buffer);

                        // 3. Send notification and save the exact msg_id returned by esp_mqtt_client_publish
                        s_job_pub_msg_id = notificar_estatus_job(current_job_id, exec_number);

                        // If publishing failed or returned an invalid ID, reboot immediately as fallback
                        if (s_job_pub_msg_id < 0) {
                            ESP_LOGE(TAG, "Error enviando notificación. Reiniciando de todos modos...");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            esp_restart();
                        }
                        return;
                    }
                }

                ESP_LOGE(TAG, "No se pudieron extraer todos los datos del Job JSON.");
            }
            break;
        }

        case MQTT_EVENT_PUBLISHED: {
            // Verify this PUBACK corresponds specifically to our Job notification message
            if (s_ota_in_progress && event->msg_id == s_job_pub_msg_id) {
                ESP_LOGI(TAG, "PUBACK recibido para Msg ID %d. Notificación confirmada por AWS.", event->msg_id);
                ESP_LOGW(TAG, "Reiniciando sistema ahora con el nuevo firmware...");
                
                vTaskDelay(pdMS_TO_TICKS(100)); // Brief pause for serial flush
                esp_restart();
                return;
            }
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Conexión perdida con el broker de AWS.");
            break;   

        default:
            break;
    }
}

void init_mqtt(void) {
	snprintf(topic_jobs, sizeof(topic_jobs), "$aws/things/%s/jobs/notify-next", AWS_THING_NAME);
    static esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(esp_mqtt_client_config_t));

    mqtt_cfg.broker.address.uri = AWS_ENDPOINT;
	
 
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

	mqtt_cfg.buffer.size = 2048;
	mqtt_cfg.buffer.out_size = 2048;
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client != NULL) {
        esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(client);
        ESP_LOGI(TAG, "MQTT Client initialized successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to initialize MQTT structure");
    }
}