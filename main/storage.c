#include "sdkconfig.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

esp_err_t read_fan_speed(int32_t *fan_speed)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        // Read
        printf("Reading fan speed from NVS ... ");
        err = nvs_get_i32(nvs_handle, "fan_speed", fan_speed);
        switch (err)
        {
        case ESP_OK:
            printf("Done\n");
            printf("fan_speed = %d\n", (int)*fan_speed);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            break;
        default:
            printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Close
        nvs_close(nvs_handle);
    }
    return ESP_OK;
}

esp_err_t write_fan_speed(int32_t fan_speed)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        // Write
        printf("Updating fan speed in NVS ... ");
        err = nvs_set_i32(nvs_handle, "fan_speed", fan_speed);

        if (err != ESP_OK)
        {
            printf("Error (%s) set!\n", esp_err_to_name(err));
        }
        else
        {
            printf("Done set fan_speed");
        }

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(nvs_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(nvs_handle);
    }
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL)
    return ESP_OK;
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