#ifndef STORAGE_H_
#define STORAGE_H_

#include <vector>
#include <stdio.h>
#include "driver/sdmmc_types.h"

class Storage {
public:
	Storage();
	virtual ~Storage();
    void Init();
	FILE* OpenRead(std::string path);
	std::vector<std::string> ListDirectoryFiles(std::string path);
	void Close(FILE* fp);

private:
	sdmmc_card_t* card;
};

#endif