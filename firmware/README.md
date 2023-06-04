# Firmware

## Old idf needed!

### install from git tag
cd ~/esp
git clone -b v3.3.6 --recursive https://github.com/espressif/esp-idf.git esp-idf-v3.3.6

### set env vars
export IDF_PATH=/Users/silvan/esp/esp-idf-v3.3.6
export PATH=$IDF_PATH/xtensa-esp32-elf/bin:$PATH    
