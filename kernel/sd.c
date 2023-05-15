#include "include/types.h"
#include "include/qspi.h"
#include "include/uart.h"
#include "include/file.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/vm.h"

#define MAX_CORES 8
#define MAX_TIMES 10000

#define TL_CLK 1000000000UL
#ifndef TL_CLK
#error Must define TL_CLK
#endif

#define F_CLK TL_CLK

static volatile uint32 * const spi = (void *)(SPI_CTRL_ADDR);
static volatile uint32 * const uart = (void *)(UART_CTRL_ADDR);

static inline uint8 spi_xfer(uint8 d)
{
	int r;
	int cnt = 0;
	REG32(spi, SPI_REG_TXFIFO) = d;
	do {
		cnt++;
		r = REG32(spi, SPI_REG_RXFIFO);
	} while (r < 0);
	return (r & 0xFF);
}

static inline uint8 sd_dummy(void)
{
	return spi_xfer(0xFF);
}

static uint8 sd_cmd(uint8 cmd, uint32 arg, uint8 crc)
{
	unsigned long n;
	uint8 r;

	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_HOLD;
	sd_dummy();
	spi_xfer(cmd);
	spi_xfer(arg >> 24);
	spi_xfer(arg >> 16);
	spi_xfer(arg >> 8);
	spi_xfer(arg);
	spi_xfer(crc);

	n = 1000;
	do {
		r = sd_dummy();
		if (!(r & 0x80)) {
			//printf("sd:cmd: %x\r\n", r);
			goto done;
		}
	} while (--n > 0);
	printf("sd_cmd: timeout\n");
done:
	return (r & 0xFF);
}

static inline void sd_cmd_end(void)
{
	sd_dummy();
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_AUTO;
}

static void sd_poweron(int f)
{
	long i;
	REG32(spi, SPI_REG_FMT) = 0x80000;
	REG32(spi, SPI_REG_CSDEF) |= 1;
	REG32(spi, SPI_REG_CSID) = 0;
	REG32(spi, SPI_REG_SCKDIV) = f;
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_OFF;
	for (i = 10; i > 0; i--) {
		sd_dummy();
	}
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_AUTO;
}

static int sd_cmd0(void)
{
	int rc;
	// printf("CMD0");
	rc = (sd_cmd(0x40, 0, 0x95) != 0x01);
	sd_cmd_end();
	return rc;
}

static int sd_cmd8(void)
{
	int rc;
	//printf("CMD8");
	rc = (sd_cmd(0x48, 0x000001AA, 0x87) != 0x01);
	sd_dummy(); /* command version; reserved */
	sd_dummy(); /* reserved */
	rc |= ((sd_dummy() & 0xF) != 0x1); /* voltage */
	rc |= (sd_dummy() != 0xAA); /* check pattern */
	sd_cmd_end();
	return rc;
}

static void sd_cmd55(void)
{
	sd_cmd(0x77, 0, 0x65);
	sd_cmd_end();
}

static int sd_acmd41(void)
{
	uint8 r;
	printf("ACMD41");
	do {
		sd_cmd55();
		r = sd_cmd(0x69, 0x40000000, 0x77); /* HCS = 1 */
	} while (r == 0x01);
	return (r != 0x00);
}

static int sd_cmd58(void)
{
	#ifdef QEMU
	return 0;
	#else
	int rc;
	printf("CMD58");
	rc = (sd_cmd(0x7A, 0, 0xFD) != 0x00);
	rc |= ((sd_dummy() & 0x80) != 0x80); /* Power up status */
	sd_dummy();
	sd_dummy();
	sd_dummy();
	sd_cmd_end();
	return rc;
	#endif
}

static int sd_cmd16(void)
{
	int rc;
	printf("CMD16");
	rc = (sd_cmd(0x50, 0x200, 0x15) != 0x00);
	sd_cmd_end();
	return rc;
}

static uint16 crc16_round(uint16 crc, uint8 data) {
	crc = (uint8)(crc >> 8) | (crc << 8);
	crc ^= data;
	crc ^= (uint8)(crc >> 4) & 0xf;
	crc ^= crc << 12;
	crc ^= (crc & 0xff) << 5;
	return crc;
}

#define SPIN_SHIFT	6
#define SPIN_UPDATE(i)	(!((i) & ((1 << SPIN_SHIFT)-1)))
#define SPIN_INDEX(i)	(((i) >> SPIN_SHIFT) & 0x3)

//static const char spinner[] = { '-', '/', '|', '\\' };


int sdRead(uint8 *buf, uint64 startSector, uint32 sectorNumber) {
	// printf("[SD Read]Read: %x\n", startSector);
	int readTimes = 0;
	int tot = 0;

start: 
	tot = sectorNumber;
	volatile uint8 *p = (void *)buf;
	int rc = 0;
	int timeout;
	uint8 x;
	#ifdef QEMU
	if (sd_cmd(0x52, startSector * 512, 0xE1) != 0x00) {
	#else
	if (sd_cmd(0x52, startSector, 0xE1) != 0x00) {
	#endif
		sd_cmd_end();
		panic("[SD Read]Read Error, retry times\n");
		return 1;
	}
	do {
		uint16 crc, crc_exp;
		long n;

		crc = 0;
		n = 512;
		timeout = MAX_TIMES;
		while (--timeout) {
			x = sd_dummy();
			if (x == 0xFE)
				break;
		}

		if (!timeout) {
			goto retry;
		}

		do {
			uint8 x = sd_dummy();
			*p++ = x;
			crc = crc16_round(crc, x);
		} while (--n > 0);

		crc_exp = ((uint16)sd_dummy() << 8);
		crc_exp |= sd_dummy();

		if (crc != crc_exp) {
			printf("\b- CRC mismatch ");
			rc = 1;
			break;
		}
	} while (--tot > 0);
	//sd_cmd_end();

	sd_cmd(0x4C, 0, 0x01);
	timeout = MAX_TIMES;
	while (--timeout) {
		x = sd_dummy();
		if (x == 0xFF) {
			break;
		}
	}
	if (!timeout) {
		goto retry;
	}
	sd_cmd_end();

	// for (int i = 0; i < 1024; i++)
	// 	printf("%x ", buf[i]);
	// printf("\nread end\n");
	// printf("[SD Read]Finish\n");
	return rc;

retry:
	readTimes++;
	if (readTimes > 10) {
		panic("[SD Read]There must be some error in sd read");
	}
	sd_cmd_end();
	goto start;
}



// This is CMD24
int sdWrite(uint8 *buf, uint64 startSector, uint32 sectorNumber) {
	// printf("[SD Write]Write: %x %d\n", startSector, sectorNumber);
	uint8 *p = buf;
	uint8 x;
	int writeTimes = 0;

	for (int i = 0; i < sectorNumber; i++) {
		uint64 now = startSector + i;
		uint8* st = p;
		writeTimes = 0;
start:	p = st;
		#ifdef QEMU
		if (sd_cmd(24 | 0x40, now * 512, 0) != 0) {
		#else
		if (sd_cmd(24 | 0x40, now, 0) != 0) {
		#endif			
			sd_cmd_end();
			panic("[SD Write]Write Error, can't use cmd24, retry times\n");
			return 1;
		}
		sd_dummy();
		sd_dummy();
		sd_dummy();
		spi_xfer(0xFE);
		int n = 512;
		do {
			spi_xfer(*p++);
		} while (--n > 0);
		int timeout = MAX_TIMES;
		while (--timeout) {
			x = sd_dummy();
			// printf("%x ", x);
			if (5 == (x & 0x1f)) {
				break;
			}
		}
		if (!timeout) {
			printf("not receive 5\n");
			goto retry;
		}
		// printf("\n");
		timeout = MAX_TIMES;
		while (--timeout) {
			x = sd_dummy();
			// printf("%x ", x);
			if (x == 0xFF) {
				break;
			}
		}
		if (!timeout) {
			printf("%x \n", x);
			printf("keep busy\n");
			goto retry;
		}
		sd_cmd_end();
	}
	return 0;
retry:
	writeTimes++;
	if (writeTimes > 10) {
		panic("[SD Write]There must be some error in sd write");
	}
	sd_cmd_end();
	goto start;
}

int sdCardRead(int isUser, uint64 dst, uint64 startAddr, uint64 n) {
	if (n & ((1 << 9) - 1)) {
		printf("[SD] Card Read error\n");
		return -1;
	}
	if (startAddr & ((1 << 9) - 1)) {
		printf("[SD] Card Read error\n");
		return -1;	
	}

	if (isUser) {
		char buf[512];
		int st = (startAddr) >> 9;
		for (int i = 0; i < n; i++) {
			sdRead((uint8*)buf, st, 1);
			copyout(myproc()->pagetable, dst, buf, 512);
			dst += 512;
			st++;
		}
		return 0;
	}
	int st = (startAddr) >> 9;
	for (int i = 0; i < n; i++) {
		sdRead((uint8*)dst, st, 1);
		dst += 512;
		st++;
	}
	return 0;
}

int sdCardWrite(int isUser, uint64 src, uint64 startAddr, uint64 n) {
	if (n & ((1 << 9) - 1)) {
		printf("[SD] Card Write error\n");
		return -1;
	}
	if (startAddr & ((1 << 9) - 1)) {
		printf("[SD] Card Write error\n");
		return -1;	
	}

	if (isUser) {
		char buf[512];
		int st = (startAddr) >> 9;
		for (int i = 0; i < n; i++) {
        	copyin(myproc()->pagetable, buf, src, 512);
			sdWrite((uint8*)buf, st, 1);
			src += 512;
			st++;
		}
		return 0;
	}
	int st = (startAddr) >> 9;
	for (int i = 0; i < n; i++) {
		sdWrite((uint8*)src, st, 1);
		src += 512;
		st++;
	}
	return 0;
}

uint8 binary[1024];
int sdTest(void) {
	// sdInit();
    for (int j = 0; j < 20; j += 2) {
        for (int i = 0; i < 1024; i++) {
            binary[i] = i & 7;
        }
        sdWrite(binary, j, 2);
        for (int i = 0; i < 1024; i++) {
            binary[i] = 0;
        }
        sdRead(binary, j, 2);
        for (int i = 0; i < 1024; i++) {
            if (binary[i] != (i & 7)) {
                panic("gg");
                break;
            }
        }
        printf("finish %d\n", j);
    }
	return 0;
}


int sdInit(void) {
	REG32(uart, UART_REG_TXCTRL) = UART_TXEN;

	sd_poweron(3000);

	int initTimes = 10;
	while (initTimes > 0 && sd_cmd0()) {
		initTimes--;
	}

	if (!initTimes) {
		panic("[SD card]CMD0 error!\n");
	}

	if (sd_cmd8()) {
		panic("[SD card]CMD8 error!\n");
	}

	if (sd_acmd41()) {
		panic("[SD card]ACMD41 error!\n");
	}

	if (sd_cmd58()) {
		panic("[SD card]CMD58 error!\n");
	}

	if (sd_cmd16()) {
		panic("[SD card]CMD16 error!\n");
	}

	printf("[SD card]SD card init finish!\n");

	REG32(spi, SPI_REG_SCKDIV) = (F_CLK / 16666666UL);
	__asm__ __volatile__ ("fence.i" : : : "memory");
	
	// devsw[0].read = sdCardRead;
	// devsw[0].write = sdCardWrite;
    sdTest();
	return 0;
}

