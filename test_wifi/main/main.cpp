#include "WebServer.h"

extern "C" {
    void app_main();
}

void app_main() {

    auto webServer = new WebServer();

    webServer->ConnectAndListen();

}
