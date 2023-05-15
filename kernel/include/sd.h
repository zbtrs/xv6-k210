#ifndef _SD_H_
#define _SD_H_
#include "types.h"

int sdInit(void);
int sdRead(uint8 *buf, uint64 startSector, uint32 sectorNumber);
int sdWrite(uint8 *buf, uint64 startSector, uint32 sectorNumber);
int sdTest(void);

#endif