#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "string.h"

#define SSID "fctl"
#define PASSPHRASE "qwerasdf"

#define STA_SSID "x1ongR"
#define STA_PASSPHRASE "qwerasdf"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_MAXIMUM_RETRY 5
#define SCAN_LIST_SIZE 10

static const char *TAG_WIFI = "WIFI";
static EventGroupHandle_t s_wifi_event_group;
static int wifi_retry_num = 0;
static wifi_config_t sta_config;
static wifi_config_t ap_config;
void start_ap(void);

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG_WIFI, "wifi sta start");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_mode_t mode;
        esp_wifi_get_mode(&mode);
        if (wifi_retry_num < WIFI_MAXIMUM_RETRY && mode == WIFI_MODE_STA)
        {
            ESP_LOGI(TAG_WIFI, "wifi connection fail, retrying...");
            esp_wifi_connect();
            wifi_retry_num++;
        }
        else
        {
            ESP_LOGI(TAG_WIFI, "maximum retry reached, starting AP");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            start_ap();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_WIFI, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_WIFI, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
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
    ESP_ERROR_CHECK(err);
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err get ap num: %s", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG_WIFI, "Total APs scanned = %u", ap_count);

    return ap_count;
}

void start_ap(void)
{
    wifi_retry_num = 0;
    ESP_LOGI(TAG_WIFI, "start wifi ap mode");

    const char *ssid = SSID;
    strncpy((char *)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid));
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
    ap_config.ap.max_connection = 8;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err set wifo mode: %s", esp_err_to_name(err));
    }

    err = esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err set wifi config: %s", esp_err_to_name(err));
    }
}

void start_sta(char *ssid, char *password)
{
    wifi_retry_num = 0;
    ESP_LOGI(TAG_WIFI, "start wifi sta mode");
    esp_err_t err = esp_wifi_disconnect();
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err disconnect wifi: %s", esp_err_to_name(err));
    }

    strncpy((char *)sta_config.sta.ssid, ssid, strlen(ssid));
    strncpy((char *)sta_config.sta.password, password, strlen(password));

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err set wifi mode: %s", esp_err_to_name(err));
    }

    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config);
    if (ESP_OK != err)
    {
        ESP_LOGE(TAG_WIFI, "err set wifi config: %s", esp_err_to_name(err));
    }
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}