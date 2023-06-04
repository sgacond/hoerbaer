#ifndef BATTERYGUARD_H_
#define BATTERYGUARD_H_

#include <esp_event_loop.h>
#include <string>
#include <esp_http_server.h>

class WebServer
{
    public:
    	WebServer();
        virtual ~WebServer();
        void ConnectAndListen();
        void CloseAndDisconnect();
        void NetworkConnected();
        void NetworkDisconnected();

    private:
        void InitWifi();
        esp_err_t HandleGetSysInfo(httpd_req_t* req);
        httpd_handle_t httpd;
};

#endif