#include <iostream>
#include <stdexcept>

#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "../../Configuration.h"
#include "Storage.h"

static const char* LOG_TAG = "STORAGE";

Storage::Storage() {
    ESP_LOGI(LOG_TAG, "Storage constructed.");
}

Storage::~Storage() {
    esp_vfs_fat_sdmmc_unmount();
    ESP_LOGI(LOG_TAG, "Storage destructed.");
}

void Storage::Init() {
    
    ESP_LOGI(LOG_TAG, "Initializing SD card.");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI_HOST_SDCARD;
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = PIN_SDCARD_MISO;
    slot_config.gpio_mosi = PIN_SDCARD_MOSI;
    slot_config.gpio_sck  = PIN_SDCARD_CLK;
    slot_config.gpio_cs   = PIN_SDCARD_CS;
    slot_config.dma_channel = 2; // 1 is used by kolbans cpp wrapper as default.
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &this->card);

    if (ret == ESP_FAIL)
        throw std::runtime_error("Failed to mount filesystem. Use only FAT32.");
    else if (ret != ESP_OK)
        throw std::runtime_error("Failed to initialize the card.");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, this->card);

}

FILE* Storage::OpenRead(std::string path) {
    ESP_LOGI(LOG_TAG, "Reading file %s", path.c_str());

    std::string fullPath("/sdcard/");
    fullPath.append(path);

    FILE* f = fopen(fullPath.c_str(), "r");
    if (f == NULL)
        throw std::runtime_error("Failed to open file for reading.");

    return f;
}

void Storage::Close(FILE* fp) {
    fclose(fp);
}