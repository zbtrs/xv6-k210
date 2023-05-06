#ifndef _SD_H_
#define _SD_H_

int sdInit(void);
int sdRead(u8 *buf, u64 startSector, u32 sectorNumber);
int sdWrite(u8 *buf, u64 startSector, u32 sectorNumber);
int sdTest(void);

#endif