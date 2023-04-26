#include "sdkconfig.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define STORAGE_NAMESPACE "storage"
#define NVS_READ_STR_LENGTH 1024

static const char *TAG_NVS = "NVS";

esp_err_t write_int(char *key, int32_t value)
{
    nvs_handle_t nvs_handle;
    esp_err_t res = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name(res));
    }
    else
    {
        // Write
        ESP_LOGI(TAG_NVS, "Updating %s in NVS ... ", key);
        res = nvs_set_i32(nvs_handle, key, value);

        if (res != ESP_OK)
        {
            ESP_LOGI(TAG_NVS, "Error (%s) set!", esp_err_to_name(res));
        }
        else
        {
            ESP_LOGI(TAG_NVS, "Done set %s", key);
        }

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        ESP_LOGI(TAG_NVS, "Committing updates in NVS ... ");
        res = nvs_commit(nvs_handle);
        if (res != ESP_OK)
        {
            ESP_LOGI(TAG_NVS, "Error (%s) commit!", esp_err_to_name(res));
        }
        else
        {
            ESP_LOGI(TAG_NVS, "Done commit");
        }

        // Close
        nvs_close(nvs_handle);
    }
    return res;
}

esp_err_t read_int(char *key, int32_t *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t res = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!\n", esp_err_to_name(res));
    }
    else
    {
        ESP_LOGI(TAG_NVS, "Reading %s from NVS ... ", key);
        res = nvs_get_i32(nvs_handle, key, value);
        switch (res)
        {
        case ESP_OK:
            ESP_LOGI(TAG_NVS, "Done, %s = %d", key, (int)*value);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG_NVS, "The value is not initialized yet!\n");
            break;
        default:
            ESP_LOGI(TAG_NVS, "Error reading %s: %s", key, esp_err_to_name(res));
        }

        nvs_close(nvs_handle);
    }
    return res;
}

esp_err_t write_str(char *key, char *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t res = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name(res));
    }
    else
    {
        // Write
        ESP_LOGI(TAG_NVS, "Updating %s in NVS ... ", key);
        res = nvs_set_str(nvs_handle, key, value);

        if (res != ESP_OK)
        {
            ESP_LOGI(TAG_NVS, "Error (%s) set!", esp_err_to_name(res));
        }
        else
        {
            ESP_LOGI(TAG_NVS, "Done set %s", key);
        }

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        ESP_LOGI(TAG_NVS, "Committing updates in NVS ... ");
        res = nvs_commit(nvs_handle);
        if (res != ESP_OK)
        {
            ESP_LOGI(TAG_NVS, "Error (%s) commit!", esp_err_to_name(res));
        }
        else
        {
            ESP_LOGI(TAG_NVS, "Done commit");
        }

        // Close
        nvs_close(nvs_handle);
    }
    return res;
}

esp_err_t read_str(char *key, char *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t res = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (res != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!\n", esp_err_to_name(res));
    }
    else
    {
        ESP_LOGI(TAG_NVS, "Reading %s from NVS ... ", key);
        size_t len = NVS_READ_STR_LENGTH;
        res = nvs_get_str(nvs_handle, key, value, &len);
        switch (res)
        {
        case ESP_OK:
            ESP_LOGI(TAG_NVS, "Done, %s = %s", key, value);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG_NVS, "The value is not initialized yet!");
            break;
        default:
            ESP_LOGI(TAG_NVS, "Error reading %s: %s", key, esp_err_to_name(res));
        }

        nvs_close(nvs_handle);
    }
    return res;
}

esp_err_t read_fan_speed(int32_t *fan_speed)
{
    return read_int("fan_speed", fan_speed);
}

esp_err_t read_ssid(char *ssid)
{
    return read_str("ssid", ssid);
}

esp_err_t write_fan_speed(int32_t fan_speed)
{
    return write_int("fan_speed", fan_speed);
}

esp_err_t write_ssid(char *ssid)
{
    return write_str("ssid", ssid);
}

void init_nvs(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
        write_fan_speed(10);
    }
    ESP_ERROR_CHECK(err);
}