#include <iostream>
#include <memory>

#include "esp_log.h"
#include <freertos/FreeRTOS.h>   // Include the base FreeRTOS definitions.
#include <GPIO.h>

#include "../Configuration.h"
#include "../storage/Storage.h"
#include "../audioplayer/AudioPlayer.h"
#include "HBI.h"


using namespace std;
using namespace ESP32CPP;

class main_application
{
public:

    void run() {

        try {
            cout << "Main started" << endl;

            auto storage = make_shared<Storage>();
            storage->Init();

            cout << "Enable peripherials" << endl;
            GPIO::setOutput(PIN_PERIPH_MAIN);
            this->enablePeripherials();

            auto hbi = make_shared<HBI>();
            hbi->start();
            hbi->setEyes(30, 30);

            // auto audioPlayer = make_unique<AudioPlayer>(move(storage));
            // audioPlayer->InitCodec();
            // audioPlayer->SetVolume(0.25f);
            // audioPlayer->PlayFile("1.MP3");

            while(1) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
        catch (std::runtime_error &e) {
            ESP_LOGE("MAIN", "Caught a runtime_error exception: %s", e.what());
            this->disablePeripherials();
        }
        catch (std::exception &e) {
            ESP_LOGE("MAIN", "Caught an exception of an unexpected type: %s", e.what());
            this->disablePeripherials();
        } 
        catch (...) {
            ESP_LOGE("MAIN", "Caught an unknown exception.");
            this->disablePeripherials();
        }
    }

private:
    void enablePeripherials() {
        GPIO::write(PIN_PERIPH_MAIN, true);
    }
    
    void disablePeripherials() {
        ESP32CPP::GPIO::write(PIN_PERIPH_MAIN, false);
    }
};

extern "C" {
    void app_main();
}

void app_main() {
    auto mainapp = main_application();
    mainapp.run();
}
