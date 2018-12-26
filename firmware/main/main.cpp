#include "HoerBaer.h"
#include "../Configuration.h"
#include "esp_sleep.h"

extern "C" {
    void app_main();
}

void app_main() {
    auto baer = new HoerBaer();
    baer->run();
    delete baer;
    
    printf("Enabling EXT1 wakeup on pin GPIO%d\n", PIN_HBI_NOSE_CLK);
    esp_sleep_enable_ext1_wakeup(1ULL << PIN_HBI_NOSE_CLK, ESP_EXT1_WAKEUP_ANY_HIGH);

    printf("Good night...\n");
    esp_deep_sleep_start();
}
