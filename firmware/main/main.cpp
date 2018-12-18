#include "HoerBaer.h"

extern "C" {
    void app_main();
}

void app_main() {
    auto baer = HoerBaer();
    baer.run();
}
