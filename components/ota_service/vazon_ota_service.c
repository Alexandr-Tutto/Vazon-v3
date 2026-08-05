#include "vazon_ota_service.h"

#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <stdlib.h>
#include <string.h>

#define OTA_MANIFEST_SIZE 2048U
#define OTA_URL_SIZE      384U
#define OTA_VERSION_SIZE  32U

typedef enum {
    OTA_REQUEST_CHECK = 0,
    OTA_REQUEST_INSTALL,
} ota_request_kind_t;

typedef struct {
    vazon_ota_service_t *service;
    ota_request_kind_t kind;
} ota_request_t;

static const char *TAG = "ota_service";
static TaskHandle_t ota_task_handle;

static bool version_valid(const char *version)
{
    if (version == NULL || version[0] == '\0') return false;
    for (const char *cursor = version; *cursor != '\0'; ++cursor) {
        const char value = *cursor;
        if (!((value >= 'a' && value <= 'z') ||
              (value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '.' ||
              value == '-' || value == '_' || value == '+')) {
            return false;
        }
    }
    return true;
}

static void report_status(vazon_ota_service_t *service, const char *state,
                          const char *version, esp_err_t error)
{
    if (error == ESP_OK) {
        ESP_LOGI(TAG, "OTA state=%s version=%s", state,
                 version == NULL ? "none" : version);
    } else {
        ESP_LOGE(TAG, "OTA state=%s error=%s", state,
                 esp_err_to_name(error));
    }
    if (service->status_callback != NULL) {
        service->status_callback(state, version, error,
                                 service->callback_context);
    }
}

static esp_err_t fetch_manifest(char *buffer, size_t buffer_size)
{
    if (CONFIG_VAZON_OTA_MANIFEST_URL[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }
    if (strncmp(CONFIG_VAZON_OTA_MANIFEST_URL, "https://", 8U) != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_http_client_config_t config = {
        .url = CONFIG_VAZON_OTA_MANIFEST_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) return ESP_ERR_NO_MEM;

    esp_err_t result = esp_http_client_open(client, 0);
    if (result != ESP_OK) goto cleanup;

    const int content_length = esp_http_client_fetch_headers(client);
    if (content_length >= (int)buffer_size) {
        result = ESP_ERR_INVALID_SIZE;
        goto close;
    }

    size_t total = 0U;
    while (total < buffer_size - 1U) {
        const int read = esp_http_client_read(
            client, buffer + total, (int)(buffer_size - 1U - total));
        if (read < 0) {
            result = ESP_FAIL;
            goto close;
        }
        if (read == 0) break;
        total += (size_t)read;
    }
    buffer[total] = '\0';
    const int status_code = esp_http_client_get_status_code(client);
    result = status_code >= 200 && status_code < 300 && total > 0U
                 ? ESP_OK
                 : ESP_ERR_INVALID_RESPONSE;

close:
    esp_http_client_close(client);
cleanup:
    esp_http_client_cleanup(client);
    return result;
}

static esp_err_t parse_manifest(const char *manifest,
                                char *version, size_t version_size,
                                char *url, size_t url_size)
{
    cJSON *root = cJSON_Parse(manifest);
    if (!cJSON_IsObject(root) || cJSON_GetArraySize(root) != 2) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    const cJSON *version_item =
        cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *url_item = cJSON_GetObjectItemCaseSensitive(root, "url");
    if (!cJSON_IsString(version_item) || version_item->valuestring == NULL ||
        !cJSON_IsString(url_item) || url_item->valuestring == NULL ||
        strncmp(url_item->valuestring, "https://", 8U) != 0) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    const size_t version_length = strlen(version_item->valuestring);
    const size_t url_length = strlen(url_item->valuestring);
    if (!version_valid(version_item->valuestring) ||
        version_length >= version_size ||
        url_length >= url_size) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(version, version_item->valuestring, version_length + 1U);
    memcpy(url, url_item->valuestring, url_length + 1U);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t install_url(const char *url)
{
    const esp_http_client_config_t http_config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
    };
    const esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };
    return esp_https_ota(&ota_config);
}

static void ota_task(void *context)
{
    ota_request_t *request = (ota_request_t *)context;
    vazon_ota_service_t *service = request->service;
    char manifest[OTA_MANIFEST_SIZE];
    char version[OTA_VERSION_SIZE];
    char url[OTA_URL_SIZE];

    report_status(service, "checking", NULL, ESP_OK);
    esp_err_t result = fetch_manifest(manifest, sizeof(manifest));
    if (result == ESP_OK) {
        result = parse_manifest(manifest, version, sizeof(version),
                                url, sizeof(url));
    }
    if (result != ESP_OK) {
        report_status(service, "failed", NULL, result);
        goto done;
    }

    const esp_app_desc_t *current = esp_app_get_description();
    if (current != NULL && strcmp(current->version, version) == 0) {
        report_status(service, "up_to_date", version, ESP_OK);
        goto done;
    }

    report_status(service, "update_available", version, ESP_OK);
    if (request->kind == OTA_REQUEST_CHECK) goto done;

    report_status(service, "downloading", version, ESP_OK);
    result = install_url(url);
    if (result != ESP_OK) {
        report_status(service, "failed", version, result);
        goto done;
    }

    report_status(service, "rebooting", version, ESP_OK);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

done:
    free(request);
    ota_task_handle = NULL;
    vTaskDelete(NULL);
}

static esp_err_t start_request(vazon_ota_service_t *service,
                               ota_request_kind_t kind)
{
    if (CONFIG_VAZON_OTA_MANIFEST_URL[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }
    if (strncmp(CONFIG_VAZON_OTA_MANIFEST_URL, "https://", 8U) != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ota_task_handle != NULL) return ESP_ERR_INVALID_STATE;
    ota_request_t *request = calloc(1, sizeof(*request));
    if (request == NULL) return ESP_ERR_NO_MEM;
    request->service = service;
    request->kind = kind;

    if (xTaskCreate(ota_task, "ota_service", CONFIG_VAZON_OTA_TASK_STACK,
                    request, 5, &ota_task_handle) != pdPASS) {
        ota_task_handle = NULL;
        free(request);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static vazon_command_result_t ota_command_handler(
    const vazon_command_t *command, void *context)
{
    vazon_ota_service_t *service = (vazon_ota_service_t *)context;
    esp_err_t result = ESP_ERR_NOT_SUPPORTED;
    if (strcmp(command->cmd, VAZON_COMMAND_OTA_CHECK) == 0) {
        result = start_request(service, OTA_REQUEST_CHECK);
    } else if (strcmp(command->cmd, VAZON_COMMAND_OTA_INSTALL) == 0) {
        result = start_request(service, OTA_REQUEST_INSTALL);
    }
    const vazon_command_result_status_t status =
        result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED :
        result == ESP_ERR_NO_MEM ? VAZON_COMMAND_RESULT_FAILED :
                                   VAZON_COMMAND_RESULT_REJECTED;
    return (vazon_command_result_t){
        .status = status,
        .error = result,
    };
}

esp_err_t vazon_ota_service_init(
    vazon_ota_service_t *service,
    vazon_ota_status_callback_t status_callback,
    void *callback_context)
{
    if (service == NULL) return ESP_ERR_INVALID_ARG;
    memset(service, 0, sizeof(*service));
    service->status_callback = status_callback;
    service->callback_context = callback_context;
    service->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_ota_service_register_command_target(
    vazon_ota_service_t *service,
    vazon_command_router_t *router)
{
    if (service == NULL || !service->initialized || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return vazon_command_router_register(router, VAZON_COMMAND_TARGET_OTA,
                                         ota_command_handler, service);
}
