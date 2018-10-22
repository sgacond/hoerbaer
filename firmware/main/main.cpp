#include <iostream>
#include <memory>
#include "esp_log.h"

#include "../storage/Storage.h"
#include "../audioplayer/AudioPlayer.h"

using namespace std;

class main_application
{
public:
    void run() {
        cout << "Main started" << endl;

        auto storage = make_shared<Storage>();
        storage->Init();

        auto audioPlayer = make_unique<AudioPlayer>(move(storage));
        audioPlayer->InitCodec();
        audioPlayer->SetVolume(0.5f);
        audioPlayer->PlayFile("3.WAV");
    }
};

extern "C" {
    void app_main();
}

void app_main() {

    try {
        auto mainapp = main_application();
        mainapp.run();
    }
    catch (std::runtime_error &e) {
        ESP_LOGE("MAIN", "Caught a runtime_error exception: %s", e.what());
    }
    catch (std::exception &e) {
        ESP_LOGE("MAIN", "Caught an exception of an unexpected type: %s", e.what());
    } 
    catch (...) {
        ESP_LOGE("MAIN", "Caught an unknown exception.");
    }

}
