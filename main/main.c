/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_wifi.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "model.h"

#define STORAGE_NAMESPACE "root_ns"

const char *ssid = "";
const char *pass = "";
uint32_t *model_buffer;

const char *create_response(iris_model_output *model_output)
{
    char setosa_buf[10], versicolor_buf[10], virginica_buf[10];
    char *string = NULL;

    sprintf(setosa_buf, "%f", model_output->setosa);
    sprintf(versicolor_buf, "%f", model_output->versicolor);
    sprintf(virginica_buf, "%f", model_output->virginica);

    cJSON *root_object = cJSON_CreateObject();
    if (root_object == NULL)
        goto end;
    cJSON *setosa = cJSON_CreateString(setosa_buf);
    if (setosa == NULL)
        goto end;
    cJSON *versicolor = cJSON_CreateString(versicolor_buf);
    if (setosa == NULL)
        goto end;
    cJSON *virginica = cJSON_CreateString(virginica_buf);
    if (setosa == NULL)
        goto end;
    cJSON_AddItemToObject(root_object, "setosa", setosa);
    cJSON_AddItemToObject(root_object, "versicolor", versicolor);
    cJSON_AddItemToObject(root_object, "virginica", virginica);
    string = cJSON_Print(root_object);

end:
    cJSON_Delete(root_object);
    return string;
}

esp_err_t get_handler(httpd_req_t *req)
{
    char querystring[250];
    httpd_req_get_url_query_str(req, querystring, 280);
    char SepalLengthCm[50], SepalWidthCm[50], PetalLengthCm[50], PetalWidthCm[50];
    httpd_query_key_value(querystring, "SepalLengthCm", SepalLengthCm, 50);
    httpd_query_key_value(querystring, "SepalWidthCm", SepalWidthCm, 50);
    httpd_query_key_value(querystring, "PetalLengthCm", PetalLengthCm, 50);
    httpd_query_key_value(querystring, "PetalWidthCm", PetalWidthCm, 50);

    float input[] = {atof(SepalLengthCm), atof(SepalWidthCm), atof(PetalLengthCm), atof(PetalWidthCm)};
    iris_model_output result = iris_model_calculate(input);

    const char *resp = create_response(&result);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void run_server()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_uri_t uri_get = {
        .uri = "/model",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL};

    printf("http server starting ...\n");

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        printf("http server has started\n");
    }
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        printf("WIFI CONNECTING....\n");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("WiFi CONNECTED\n");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("WiFi lost connection\n");
        esp_wifi_connect();
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        run_server();
    }
}

void wifi_connection()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
        }};
    strcpy((char *)wifi_configuration.sta.ssid, ssid);
    strcpy((char *)wifi_configuration.sta.password, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
}

void init_model()
{
    nvs_handle_t my_handle;

    ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle));

    size_t required_size = 0;
    ESP_ERROR_CHECK(nvs_get_blob(my_handle, "model", NULL, &required_size));

    model_buffer = malloc(required_size + sizeof(uint32_t));
    if (required_size > 0)
    {
        esp_err_t err = nvs_get_blob(my_handle, "model", model_buffer, &required_size);
        if (err != ESP_OK)
        {
            printf("error for reading file");
            free(model_buffer);
            return;
        }
    }

    iris_model_load(model_buffer);

    printf("Model was loaded successfully\n");
}

void app_main(void)
{
    printf("Initializing my device ...\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    init_model();
    wifi_connection();
    for (;;) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    free(model_buffer);
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
