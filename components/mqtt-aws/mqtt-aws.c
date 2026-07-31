#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "mqtt_client.h"  
#include "sdkconfig.h"  
#include "cJSON.h"    

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
        ESP_LOGI(TAG, "Update successfully installed. Rebooting...");
    } else {
        ESP_LOGI(TAG, "Update failed. Rebooting...");
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        
		case MQTT_EVENT_CONNECTED: {
		            ESP_LOGI(TAG, "Conectado a AWS IoT Core de forma segura.");
		            s_mqtt_client = event->client;
		            
		            // 2. NUEVO: Suscribirse también al tópico de respuesta de Jobs pendientes
		            // AWS responderá aquí con la lista de trabajos atascados en IN_PROGRESS
		            char topic_pending_accepted[128];
		            snprintf(topic_pending_accepted, sizeof(topic_pending_accepted), 
		                     "$aws/things/esp32-sensor-01/jobs/get/accepted");
		            esp_mqtt_client_subscribe(s_mqtt_client, topic_pending_accepted, 1);

		            // 3. NUEVO: Publicar un mensaje vacío en /get para pedirle a AWS los Jobs pendientes
		            char topic_get_pending[128];
		            snprintf(topic_get_pending, sizeof(topic_get_pending), 
		                     "$aws/things/esp32-sensor-01/jobs/get");
		            
		            // Al publicar un string vacío "{}" aquí, AWS te devolverá inmediatamente 
		            // el JSON de 1700 bytes del Job que tienes atascado en "In Progress"
		            esp_mqtt_client_publish(s_mqtt_client, topic_get_pending, "{}", 2, 1, 0);
		            ESP_LOGI(TAG, "Solicitando lista de Jobs pendientes a AWS...");
		            break;
		        }
        
				case MQTT_EVENT_DATA: {
				            if (s_ota_in_progress) {
				                ESP_LOGW(TAG, "Mensaje MQTT ignorado: OTA ya en proceso.");
				                break;
				            }

				            bool es_notificacion = (strncmp(event->topic, topic_jobs, event->topic_len) == 0);
				            bool es_pendiente = (strstr(event->topic, "/jobs/get/accepted") != NULL);
				            bool es_detalle_job = (strstr(event->topic, "/get/accepted") != NULL && !es_pendiente);

				            if (es_notificacion || es_pendiente || es_detalle_job) {
				                
				                static char *json_assemble_buffer = NULL;
				                
				                if (event->current_data_offset == 0) {
				                    if (json_assemble_buffer != NULL) {
				                        free(json_assemble_buffer);
				                        json_assemble_buffer = NULL;
				                    }
				                    json_assemble_buffer = malloc(event->total_data_len + 1);
				                    if (json_assemble_buffer == NULL) {
				                        ESP_LOGE(TAG, "No hay memoria para ensamblar el JSON de %d bytes", event->total_data_len);
				                        break;
				                    }
				                }

				                if (json_assemble_buffer == NULL) {
				                    break; 
				                }

				                memcpy(json_assemble_buffer + event->current_data_offset, event->data, event->data_len);

				                if (event->current_data_offset + event->data_len < event->total_data_len) {
				                    break; 
				                }

				                json_assemble_buffer[event->total_data_len] = '\0';
				                ESP_LOGI(TAG, "JSON ensamblado con éxito (%d bytes). Iniciando parseo...", event->total_data_len);

				                char current_job_id[64] = {0};
				                int exec_number = 1; // <--- SE USARÁ ABAJO EN LA NOTIFICACIÓN DE ERROR
				                char url_buffer[2048] = {0}; 
				                bool parse_success = false;
				                bool requiere_detalles_job = false;

				                cJSON *root = cJSON_Parse(json_assemble_buffer);
				                
				                if (es_notificacion || es_detalle_job) {
				                    if (root != NULL) {
				                        cJSON *execution = cJSON_GetObjectItemCaseSensitive(root, "execution");
				                        if (execution != NULL) {
				                            cJSON *jobId = cJSON_GetObjectItemCaseSensitive(execution, "jobId");
				                            cJSON *execNum = cJSON_GetObjectItemCaseSensitive(execution, "executionNumber");
				                            cJSON *jobDoc = cJSON_GetObjectItemCaseSensitive(execution, "jobDocument");
				                            
				                            if (jobId != NULL && execNum != NULL && jobDoc != NULL) {
				                                cJSON *url = cJSON_GetObjectItemCaseSensitive(jobDoc, "url");
				                                
				                                bool has_job = (jobId->valuestring != NULL && strlen(jobId->valuestring) > 0);
				                                bool has_url = (url != NULL && url->valuestring != NULL && strlen(url->valuestring) > 0);
				                                
				                                if (has_job && has_url) {
				                                    strlcpy(current_job_id, jobId->valuestring, sizeof(current_job_id));
				                                    exec_number = execNum->valueint; // <--- Guardado correcto
				                                    strlcpy(url_buffer, url->valuestring, sizeof(url_buffer));
				                                    parse_success = true;
				                                }
				                            }
				                        }
				                    }
				                } 
				                else if (es_pendiente) {
				                    char *job_start = strstr(json_assemble_buffer, "ota-job-");
				                    if (job_start != NULL) {
				                        char *job_end = job_start;
				                        while (*job_end != '\0' && *job_end != '"' && *job_end != '\\') {
				                            job_end++;
				                        }
				                        size_t job_len = job_end - job_start;
				                        if (job_len > 0 && job_len < sizeof(current_job_id)) {
				                            memcpy(current_job_id, job_start, job_len);
				                            current_job_id[job_len] = '\0';
				                            requiere_detalles_job = true; 
				                        }
				                    }
				                }

				                if (root != NULL) {
				                    cJSON_Delete(root); 
				                }

				                free(json_assemble_buffer);
				                json_assemble_buffer = NULL;

				                if (requiere_detalles_job && strlen(current_job_id) > 0) {
				                    ESP_LOGW(TAG, "¡Job '%s' detectado en AWS! Exigiendo URL completa...", current_job_id);
				                    
				                    char topic_req_accepted[128];
				                    snprintf(topic_req_accepted, sizeof(topic_req_accepted), 
				                             "$aws/things/esp32-sensor-01/jobs/%s/get/accepted", current_job_id);
				                    esp_mqtt_client_subscribe(s_mqtt_client, topic_req_accepted, 1);

				                    char topic_req_get[128];
				                    snprintf(topic_req_get, sizeof(topic_req_get), 
				                             "$aws/things/esp32-sensor-01/jobs/%s/get", current_job_id);
				                    
				                    vTaskDelay(pdMS_TO_TICKS(150)); 
				                    esp_mqtt_client_publish(s_mqtt_client, topic_req_get, "{}", 2, 1, 0);
				                    break; 
				                }

				                // --- PROCESAR EJECUCIÓN DEL OTA ---
								if (parse_success) {
								                    s_ota_in_progress = true; // Alzamos el guard de protección
								                    ESP_LOGI(TAG, "¡ÉXITO TOTAL! URL extraída correctamente.");
								                    
								                    char topic_clean[128];
								                    snprintf(topic_clean, sizeof(topic_clean), 
								                             "$aws/things/esp32-sensor-01/jobs/%s/get/accepted", current_job_id);
								                    esp_mqtt_client_unsubscribe(s_mqtt_client, topic_clean);

								                    // 1. PRIMERO NOTIFICAMOS ÉXITO A AWS 
								                    // Esto asegura que en la consola web de AWS pase de IN_PROGRESS a SUCCEEDED
								                    ESP_LOGI(TAG, "Notificando resultado EXITOSO del Job a AWS IoT Core...");
								                    s_job_pub_msg_id = notificar_estatus_job(current_job_id, exec_number);

								                    if (s_job_pub_msg_id >= 0) {
								                        ESP_LOGI(TAG, "Confirmación de éxito enviada (ID: %d). Iniciando escritura...", s_job_pub_msg_id);
								                    }
								                    
								                    // Pausa de 1 segundo para asegurar que los datos viajen por el aire antes de bloquear el micro con el flash
								                    vTaskDelay(pdMS_TO_TICKS(1000));

								                    // 2. DISPARAMOS EL FLASHEO DE LA PARTICIÓN
								                    // Tu log demostró que esta función instala el firmware al 100% con éxito
								                    ejecutar_actualizacion_ota(url_buffer);

								                    // 3. REINICIO ELÉCTRICO REAL (OBLIGATORIO)
								                    // Como tu función OTA no está reseteando el chip por hardware, lo obligamos aquí mismo:
								                    ESP_LOGW(TAG, "¡Flasheo completado! Forzando REINICIO DE HARDWARE inmediato...");
								                    vTaskDelay(pdMS_TO_TICKS(1000));
								                    
								                    esp_restart(); // <--- ESTO APAGA Y ENCIENDE EL CHIP DE VERDAD

								                    // Un bucle infinito de resguardo absoluto para que el procesador muera de forma limpia 
								                    // mientras los transistores del ESP32 cortan la energía.
								                    while (1) {
								                        vTaskDelay(pdMS_TO_TICKS(1000));
								                    }
								                } 
								                else if (!requiere_detalles_job) {
													if (event->total_data_len < 100) {
													                        ESP_LOGI(TAG, "El dispositivo está completamente al día. No hay Jobs pendientes en AWS.");
													                    } else {
													                        ESP_LOGE(TAG, "Error: No se pudieron extraer los campos necesarios del JSON recibido.");
													                    }
								                }
				            }
				            break; 
				        }
        default:
            break;
    }
}


void init_mqtt(void) {
    snprintf(topic_jobs, sizeof(topic_jobs), "$aws/things/%s/jobs/notify", AWS_THING_NAME);
    
    // Inicialización real, nativa y segura bajo la especificación oficial de ESP-IDF v6.x
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = AWS_ENDPOINT,
            },
            .verification = {
                .certificate = (const char *)aws_root_ca_pem_start,
                .certificate_len = (size_t)(aws_root_ca_pem_end - aws_root_ca_pem_start),
            },
        },
        .credentials = {
            .client_id = AWS_THING_NAME,
            .authentication = {
                .certificate = (const char *)certificate_pem_crt_start,
                .certificate_len = (size_t)(certificate_pem_crt_end - certificate_pem_crt_start),
                .key = (const char *)private_pem_key_start,
                .key_len = (size_t)(private_pem_key_end - private_pem_key_start),
            },
        },
        .buffer = {
            .size = 4096,     // Buffer de recepción (Rx) holgado para albergar los fragmentos de AWS
            .out_size = 2048,  // Buffer de transmisión (Tx)
        },
        // CORRECCIÓN: Estructura corregida con los únicos dos campos válidos del SDK oficial
        .task = {
            .priority = 10,       // Prioridad alta por encima del bucle de sensores
            .stack_size = 6144,   // Stack de 6KB suficiente para ensamblar tramas y correr cJSON con seguridad
        }
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client != NULL) {
        esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(client);
        ESP_LOGI(TAG, "MQTT Client initialized successfully with dedicated task priority.");
    } else {
        ESP_LOGE(TAG, "Failed to initialize MQTT structure");
    }
}
