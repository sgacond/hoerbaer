#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>

#include "WebServer.h"

static const char* LOG_TAG = "WEB";

WebServer::WebServer() {
    
}

WebServer::~WebServer() {
}

void WebServer::ConnectAndListen() {
    ESP_LOGI(LOG_TAG, "Hello from connect");
    this->InitWifi();
}

void WebServer::CloseAndDisconnect() {
    ESP_LOGI(LOG_TAG, "Adjos from disconnect");
}

esp_err_t WebServer::HandleGetSysInfo(httpd_req_t* req) {
    return ESP_OK;
}

void WebServer::NetworkConnected() {

    // start webserver
    this->httpd = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(LOG_TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&this->httpd, &config) != ESP_OK) {
        ESP_LOGI(LOG_TAG, "Error starting server!");
        return;
    }

    // Set URI handlers
    ESP_LOGI(LOG_TAG, "Registering URI handlers");

    // esp_err_t (WebServer::*)(httpd_req_t*) {aka int (WebServer::*)(httpd_req*)}
    // esp_err_t            (*)(httpd_req_t*) {aka int            (*)(httpd_req*)}

    int (WebServer::* handlerptr)(httpd_req*) = &WebServer::HandleGetSysInfo;

    httpd_uri_t sysInfoUri {
        .uri      = "/api/info",
        .method   = HTTP_GET,
        .handler  = handlerptr,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(this->httpd, &sysInfoUri);
}

void WebServer::NetworkDisconnected() {

    if(this->httpd == NULL)
        return;
    
    httpd_stop(this->httpd);
    this->httpd = NULL;

}

static esp_err_t WifiEventHandler(void *ctx, system_event_t *event) {
    WebServer *server = (WebServer *) ctx;
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_START");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ESP_LOGI(LOG_TAG, "Got IP: '%s'",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            server->NetworkConnected();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            ESP_ERROR_CHECK(esp_wifi_connect());
            server->NetworkDisconnected();
            break;
        default:
            break;
    }
    return ESP_OK;
}

void WebServer::InitWifi() {

    ESP_ERROR_CHECK(nvs_flash_init());

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(WifiEventHandler, this));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    std::string ssid("Kiva");
    std::string password("pandabaer");

    wifi_config_t wifi_config;
	::memset(&wifi_config, 0, sizeof(wifi_config));
	::memcpy(wifi_config.sta.ssid, ssid.data(), ssid.size());
	::memcpy(wifi_config.sta.password, password.data(), password.size());

    ESP_LOGI(LOG_TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

}

