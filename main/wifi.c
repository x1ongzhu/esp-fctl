#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "string.h"

#define AP_SSID "fctl"
#define AP_PWD "12345678"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_MAXIMUM_RETRY 5
#define SCAN_LIST_SIZE 10

static const char *TAG_WIFI = "WIFI";
static EventGroupHandle_t s_wifi_event_group;
static int wifi_retry_num = 0;
static wifi_config_t wifi_config;
void stop_ap(void);

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t err;
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_SCAN_DONE:
            ESP_LOGI(TAG_WIFI, "scan done");
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG_WIFI, "sta start");
            err = esp_wifi_connect();
            if (ESP_OK != err)
            {
                ESP_LOGE(TAG_WIFI, "err connect: %s", esp_err_to_name(err));
            }
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG_WIFI, "sta stop");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG_WIFI, "sta connected");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG_WIFI, "sta disconnected");
            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);
            if (wifi_retry_num < WIFI_MAXIMUM_RETRY)
            {
                ESP_LOGI(TAG_WIFI, "try to reconnect");
                err = esp_wifi_connect();
                if (ESP_OK != err)
                {
                    ESP_LOGE(TAG_WIFI, "err connect: %s", esp_err_to_name(err));
                }
                wifi_retry_num++;
            }
            else
            {
                ESP_LOGI(TAG_WIFI, "maximum retry reached, stop retrying");
            }
            break;
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGI(TAG_WIFI, "sta authmode change");
            break;
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG_WIFI, "ap start");
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG_WIFI, "ap stop");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
        {
            ESP_LOGI(TAG_WIFI, "ap sta connected");
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG_WIFI, "station " MACSTR " join, AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            ESP_LOGI(TAG_WIFI, "ap sta disconnected");
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG_WIFI, "station " MACSTR " leave, AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }
        default:
            ESP_LOGI(TAG_WIFI, "unknown wifi event: %d", (int)event_id);
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            wifi_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

uint16_t wifi_scan(wifi_ap_record_t *ap_info, uint16_t size)
{
    ESP_LOGI(TAG_WIFI, "start wifi scan");
    esp_err_t err = esp_wifi_clear_ap_list();
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err clear ap list: %s", esp_err_to_name(err));
    }

    err = esp_wifi_scan_start(NULL, true);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err start scan: %s", esp_err_to_name(err));
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_records(&size, ap_info);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err get ap records: %s", esp_err_to_name(err));
    }

    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err get ap num: %s", esp_err_to_name(err));
    }
    ESP_LOGI(TAG_WIFI, "Total APs scanned = %u", ap_count);

    return ap_count;
}

void config_sta(char *ssid, char *password)
{
    wifi_retry_num = 0;
    ESP_LOGI(TAG_WIFI, "start wifi sta mode");

    strncpy((char *)wifi_config.sta.ssid, ssid, 32);
    strncpy((char *)wifi_config.sta.password, password, 64);
    ESP_LOGI(TAG_WIFI, "ssid: %s, password: %s", (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);

    esp_err_t err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err set wifi config: %s", esp_err_to_name(err));
    }

    err = esp_wifi_connect();
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err connect: %s", esp_err_to_name(err));
    }
}

void stop_ap(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

void init_wifi(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_get_config(ESP_IF_WIFI_AP, &wifi_config);

    const char *ssid = AP_SSID;
    const char *password = AP_PWD;
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.max_connection = 16;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
}
