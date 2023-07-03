/*
 * @brief LPC18xx/43xx SDIO Card driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

// #include "chip.h"
#include "include/sdio_18xx_43xx.h"
#include "include/sdif_18xx_43xx.h"
#include "include/types.h"
#include "include/printf.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define SDIO_CMD_INT_MSK    0xA146       /* Interrupts to be enabled for CMD */
#define SDIO_DATA_INT_MSK   0xBE88       /* Interrupts to enable for data transfer */
#define SDIO_CARD_INT_MSK   (1UL << 16)  /* SDIO Card interrupt */

static struct
{
	void (*wake_evt)(LPC_SDMMC_T *pSDMMC, uint32 event, uint32 arg);
	uint32 (*wait_evt)(LPC_SDMMC_T *pSDMMC, uint32 event, uint32 arg);
	uint32 flag;
	uint32 response[4];
	int fnum;
	uint16 blkSz[8];     /* Block size setting for the 8- function blocks */
	sdif_device sdev;      /* SDIO interface device structure */
}sdio_context, *sdioif = &sdio_context;

uint32 wait_for_sdio_irq(LPC_SDMMC_T *pSDMMC)
{
	uint32 rintst;
	while (1)
	{
		rintst = pSDMMC->RINTSTS;
		printf("rintst: %p\n", rintst);
		if (rintst & 0xffff0004)
		{
			break;
		}
	}
	SDIO_IRQHandler();
	return 0;
}

uint32 wait_for_read_irq(LPC_SDMMC_T *pSDMMC)
{
	uint32 rintst;
	while (1)
	{
		rintst = pSDMMC->RINTSTS;
		printf("rintst: %p\n", rintst);
		if (rintst & 0x20)
		{
			break;
		}
	}
	SDIO_IRQHandler();
	return 0;
}

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Set the SDIO Card voltage level to 3v3 */
static int SDIO_Card_SetVoltage(LPC_SDMMC_T *pSDMMC)
{
	int ret, i;
	uint32 val;

	printf("%p\n",pSDMMC->RINTSTS);
	printf("responese: %p\n", pSDMMC->RESP0);
	ret = SDIO_Send_Command(pSDMMC, CMD5, 0);
	// if (ret) return ret;
	val = sdioif->response[0];
	printf("response: %p\n", val);
	/* Number of functions supported by the card */
	sdioif->fnum = (val >> 28) & 7;

	/* Check number of I/O functions*/
	if(sdioif->fnum == 0) {
		/* Number of I/O functions */
		return SDIO_ERR_FNUM;
	}

	/* ---- check OCR ---- */
	if((val & SDIO_VOLT_3_3) == 0){
		/* invalid voltage */
		return SDIO_ERR_VOLT;
	}

	/* ==== send CMD5 write new voltage  === */
	for(i = 0; i < 100; i++){
		ret = SDIO_Send_Command(pSDMMC, CMD5, SDIO_VOLT_3_3);
		if (ret) return ret;
		val = sdioif->response[0];

		/* Is card ready ? */
		if(val & (1UL << 31)){
			break;
		}

		sdioif->wait_evt(pSDMMC, SDIO_WAIT_DELAY, 10);
	}

	/* ==== Check C bit  ==== */
	if(val & (1UL << 31)){
		return 0;
	}

	return SDIO_ERR_VOLT; /* error end */
}

/* Set SDIO Card RCA */
static int SDIO_CARD_SetRCA(LPC_SDMMC_T *pSDMMC)
{
	int ret;

	/* ==== send CMD3 get RCA  ==== */
	ret = SDIO_Send_Command(pSDMMC, CMD3, 0);
	if (ret) return ret;

	/* R6 response to CMD3 */
	if((sdioif->response[0] & 0x0000e000) != 0){
						/* COM_CRC_ERROR */
						/* ILLEGAL_CRC_ERROR */
						/* ERROR */
		return SDIO_ERR_RCA;
	}

	/* change card state */
	sdioif->flag |= SDIO_POWER_INIT;

	/* New published RCA */
	sdioif->response[0] &= 0xffff0000;

	/* ==== change state to Stanby State ==== */
	return SDIO_Send_Command(pSDMMC, CMD7, sdioif->response[0]);
}

/* Set the Clock speed and mode [1/4 bit] of the card */
static int SDIO_Card_SetMode(LPC_SDMMC_T *pSDMMC, uint32 clk, int mode_4bit)
{
	int ret;
	uint32 val;

	// Chip_SDIF_SetClock(pSDMMC, Chip_Clock_GetBaseClocktHz(CLK_BASE_SDIO), clk);

	if (!mode_4bit)
		return 0;

	val = 0x02;
	ret = SDIO_WriteRead_Direct(pSDMMC, SDIO_AREA_CIA, 0x07, &val);
	if (ret) return ret;

	if (val & 0x02) {
		Chip_SDIF_SetCardType(pSDMMC, MCI_CTYPE_4BIT);
	}
	return 0;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/
/* Set the block size of a function */
int SDIO_Card_SetBlockSize(LPC_SDMMC_T *pSDMMC, uint32 func, uint32 blkSize)
{
	int ret;
	uint32 tmp, asz;
	if (func > sdioif->fnum)
		return SDIO_ERR_INVFUNC;

	if (blkSize > 2048)
		return SDIO_ERR_INVARG;

	tmp = blkSize & 0xFF;
	ret = SDIO_WriteRead_Direct(pSDMMC, SDIO_AREA_CIA, (func << 8) + 0x10, &tmp);
	if (ret) return ret;
	asz = tmp;

	tmp = blkSize >> 8;
	ret = SDIO_WriteRead_Direct(pSDMMC, SDIO_AREA_CIA, (func << 8) + 0x11, &tmp);
	if (ret) return ret;
	asz |= tmp << 8;
	sdioif->blkSz[func] = asz;
	return 0;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/
/* Set the block size of a function */
int SD_Card_SetBlockSize(LPC_SDMMC_T *pSDMMC, uint32 blkSize, uint32 rca)
{
	uint32 val;
	SDIO_Send_Command(pSDMMC, CMD7, rca);
	val = sdioif->response[0];
	printf("response: %p\n", val);

	SDIO_Send_Command(pSDMMC, CMD16, blkSize);
	val = sdioif->response[0];
	printf("response: %p\n", val);

	pSDMMC->BLKSIZ = 512;

}

/* Get the block size of a particular function */
uint32 SDIO_Card_GetBlockSize(LPC_SDMMC_T *pSDMMC, uint32 func)
{
	if (func > sdioif->fnum)
		return 0;

	return sdioif->blkSz[func];
}

/* Write data to SDIO Card */
int SDIO_Card_WriteData(LPC_SDMMC_T *pSDMMC, uint32 func,
	uint32 dest_addr, const uint8 *src_addr,
	uint32 size, uint32 flags)
{
	int ret;
	uint32 bs = size, bsize = size;
	uint32 cmd = CMD53 | (1UL << 10);

	if (func > sdioif->fnum)
		return SDIO_ERR_INVFUNC;

	if (bsize > 512 || bsize == 0)
		return SDIO_ERR_INVARG;

	if (flags & SDIO_MODE_BLOCK) {
		uint32 bs = SDIO_Card_GetBlockSize(pSDMMC, func);
		if (!bs) return SDIO_ERR_INVARG;
		size *= bs;
	}

	/* Set Block Size */
	Chip_SDIF_SetBlkSize(pSDMMC, bs);

	/* set number of bytes to read */
	Chip_SDIF_SetByteCnt(pSDMMC, size);

	sdioif->wait_evt(pSDMMC, SDIO_START_DATA, 0);
	Chip_SDIF_DmaSetup(pSDMMC, &sdioif->sdev, (uint64) src_addr, size);

	ret = SDIO_Send_Command(pSDMMC, cmd, (func << 28) | (dest_addr << 9) | (bsize & 0x1FF) | (1UL << 31) | (flags & (0x3 << 26)));
	if (ret) return ret;

	/* Check response for errors */
	if(sdioif->response[0] & 0xcb00){
						/* COM_CRC_ERROR */
						/* ILLEGAL_CRC_ERROR */
						/* ERROR */
						/* RFU FUNCTION_NUMBER */
						/* OUT_OF_RANGE */
		/* Response flag error */
		return SDIO_ERR_READWRITE;
	}
	return sdioif->wait_evt(pSDMMC, SDIO_WAIT_DATA, 0);
}


/* Write data to SDIO Card */
int SDIO_Card_ReadData(LPC_SDMMC_T *pSDMMC, uint32 func, uint8 *dest_addr, uint32 src_addr, uint32 size, uint32 flags)
{
	int ret;
	uint32 bs = size, bsize = size;
	uint32 cmd = CMD53;

	if (func > sdioif->fnum)
		return SDIO_ERR_INVFUNC;

	if (bsize > 512 || bsize == 0)
		return SDIO_ERR_INVARG;

	if (flags & SDIO_MODE_BLOCK) {
		bs = SDIO_Card_GetBlockSize(pSDMMC, func);
		if (!bs) return SDIO_ERR_INVARG;
		size *= bs;
	}
	/* Set the block size */
	Chip_SDIF_SetBlkSize(pSDMMC, bs);

	/* set number of bytes to read */
	Chip_SDIF_SetByteCnt(pSDMMC, size);

	sdioif->wait_evt(pSDMMC, SDIO_START_DATA, 0);
	Chip_SDIF_DmaSetup(pSDMMC, &sdioif->sdev, (uint64) dest_addr, size);

	ret = SDIO_Send_Command(pSDMMC, cmd | (1 << 13), (func << 28) | (src_addr << 9) | (bsize & 0x1FF) | (flags & (0x3 << 26)));
	if (ret) return ret;

	/* Check response for errors */
	if(sdioif->response[0] & 0xcb00){
						/* COM_CRC_ERROR */
						/* ILLEGAL_CRC_ERROR */
						/* ERROR */
						/* RFU FUNCTION_NUMBER */
						/* OUT_OF_RANGE */
		/* Response flag error */
		return SDIO_ERR_READWRITE;
	}

	return sdioif->wait_evt(pSDMMC, SDIO_WAIT_DATA, 0);
}

int SD_Card_ReadData(LPC_SDMMC_T *pSDMMC, uint32 size)
{
	pSDMMC->BLKSIZ = 512;
	if (size % 512)
	{
		pSDMMC->BYTCNT = ((size % 512) + 1) * 512;
	}
	else
	{
		pSDMMC->BYTCNT = size;
	}
	
	
}

/* Enable SDIO function interrupt */
int SDIO_Card_EnableInt(LPC_SDMMC_T *pSDMMC, uint32 func)
{
	int ret;
	uint32 val;

	if (func > sdioif->fnum)
		return SDIO_ERR_INVFUNC;

	ret = SDIO_Read_Direct(pSDMMC, SDIO_AREA_CIA, 0x04, &val);
	if (ret) return ret;
	val |= (1 << func) | 1;
	ret = SDIO_Write_Direct(pSDMMC, SDIO_AREA_CIA, 0x04, val);
	if (ret) return ret;
	pSDMMC->INTMASK |= SDIO_CARD_INT_MSK;

	return 0;
}

/* Disable SDIO function interrupt */
int SDIO_Card_DisableInt(LPC_SDMMC_T *pSDMMC, uint32 func)
{
	int ret;
	uint32 val;

	if (func > sdioif->fnum)
		return SDIO_ERR_INVFUNC;

	ret = SDIO_Read_Direct(pSDMMC, SDIO_AREA_CIA, 0x04, &val);
	if (ret) return ret;
	val &= ~(1 << func);

	/* Disable master interrupt if it is the only thing enabled */
	if (val == 1)
		val = 0;
	ret = SDIO_Write_Direct(pSDMMC, SDIO_AREA_CIA, 0x04, val);
	if (ret) return ret;
	if (!val)
		pSDMMC->INTMASK &= ~SDIO_CARD_INT_MSK;

	return 0;
}

/* Initialize the SDIO card */
int SDIO_Card_Init(LPC_SDMMC_T *pSDMMC, uint32 freq)
{
	int ret;
	uint32 val;

	/* Set Clock to 400KHz */
	Chip_SDIF_SetCardType(pSDMMC, 0);
	Chip_SDIF_SetClock(pSDMMC, 200000000, freq);
	printf("arrive a0\n");
	sdioif->wait_evt(pSDMMC, SDIO_WAIT_DELAY, 100); /* Wait for card to wake up */

	printf("arrive a\n");
	if (sdioif->flag & SDIO_POWER_INIT) {
		/* Write to the Reset Bit */
		ret = SDIO_Write_Direct(pSDMMC, SDIO_AREA_CIA, 0x06, 0x08);
		if (ret) return ret;
	}
	printf("arrive b\n");


	/* Set Voltage level to 3v3 */
	ret = SDIO_Card_SetVoltage(pSDMMC);
	if (ret) return ret;
	printf("arrive c\n");
	/* Set the RCA */
	ret = SDIO_CARD_SetRCA(pSDMMC);
	printf("arrive d\n");
	if (ret) return ret;

	/* ==== check card capability ==== */
	val = 0x02;
	ret = SDIO_WriteRead_Direct(pSDMMC, SDIO_AREA_CIA, 0x13, &val);
	if (ret) return ret;
	printf("arrive e\n");
	/* FIXME: Verify */
	/* FIFO threshold settings for DMA, DMA burst of 4,   FIFO watermark at 16 */
	pSDMMC->FIFOTH = MCI_FIFOTH_DMA_MTS_1 | MCI_FIFOTH_RX_WM(0) | MCI_FIFOTH_TX_WM(1);

	/* Enable internal DMA, burst size of 4, fixed burst */
	pSDMMC->BMOD = MCI_BMOD_DE | MCI_BMOD_PBL1 | MCI_BMOD_DSL(0);

	/* High Speed Support? */
	if ((val & 0x03) == 3) {
		return SDIO_Card_SetMode(pSDMMC, SDIO_CLK_HISPEED, 1);
	}
	printf("arrive f\n");
	ret = SDIO_Read_Direct(pSDMMC, SDIO_AREA_CIA, 0x08, &val);
	if (ret) return ret;
	printf("arrive g\n");
	/* Full Speed Support? */
	if (val & SDIO_CCCR_LSC) {
		return SDIO_Card_SetMode(pSDMMC, SDIO_CLK_FULLSPEED, 1);
	}
	printf("arrive h\n");
	/* Low Speed Card */
	return SDIO_Card_SetMode(pSDMMC, SDIO_CLK_LOWSPEED, val & SDIO_CCCR_4BLS);
}

/*
	Initialize sd card
	return value of rca
	by comedymaker
*/
uint32 SD_Card_Init(LPC_SDMMC_T *pSDMMC, uint32 freq)
{
	int ret;
	uint32 val;
	uint32 rca;
	/* Set Clock to 400KHz */
	Chip_SDIF_SetCardType(pSDMMC, 0);
	Chip_SDIF_SetClock(pSDMMC, 200000000, freq);
	printf("arrive a0\n");
	sdioif->wait_evt(pSDMMC, SDIO_WAIT_DELAY, 100); /* Wait for card to wake up */

	printf("arrive a\n");
	printf("RINTSTS: %p\n",pSDMMC->RINTSTS);
	printf("responese: %p\n", pSDMMC->RESP0);
	ret = SDIO_Send_Command(pSDMMC, CMD5, 0);
	// if (ret) return ret;
	val = sdioif->response[0];
	printf("response: %p\n", val);
	printf("arrive b\n");

	printf("RINTSTS: %p\n",pSDMMC->RINTSTS);
	ret = SDIO_Send_Command(pSDMMC, CMD0, 0);
	// if (ret) return ret;
	val = sdioif->response[0];
	printf("response: %p\n", val);

	printf("RINTSTS: %p\n",pSDMMC->RINTSTS);

	SDIO_Send_Command(pSDMMC, CMD8, 0x1aa);
	val = sdioif->response[0];
	printf("response: %p\n", val);
	printf("arrive c\n");


	do
	{
		SDIO_Send_Command(pSDMMC, CMD55, 0);
		SDIO_Send_Command(pSDMMC, ACMD41, 0x40100000);
		val = sdioif->response[0];
	} while ((val & 0x80000000) == 0);
	
	printf("response: %p\n", val);
	printf("arrive d\n");

	SDIO_Send_Command(pSDMMC, CMD2, 0);
	printf("response3: %p\n", sdioif->response[3]);
	printf("response3: %p\n", sdioif->response[2]);
	printf("response3: %p\n", sdioif->response[1]);
	printf("response3: %p\n", sdioif->response[0]);
	printf("arrive e\n");

	SDIO_Send_Command(pSDMMC, CMD3, 0);
	val = sdioif->response[0];
	printf("response: %p\n", val);
	printf("arrive e\n");
	
	rca = (val & 0xffff0000);
	printf("rca: %p\n", rca);

	Chip_SDIF_SetClock(pSDMMC, 200000000, 25000000);
	return rca;

}

/* Write given data to register space of the CARD */
int SDIO_Write_Direct(LPC_SDMMC_T *pSDMMC, uint32 func, uint32 addr, uint32 data)
{
	int ret;

	ret = SDIO_Send_Command(pSDMMC, CMD52, (func << 28) | (addr << 9) | (data & 0xFF) | (1UL << 31));
	if (ret) return ret;

	/* Check response for errors */
	if(sdioif->response[0] & 0xcb00){
						/* COM_CRC_ERROR */
						/* ILLEGAL_CRC_ERROR */
						/* ERROR */
						/* RFU FUNCTION_NUMBER */
						/* OUT_OF_RANGE */
		/* Response flag error */
		return SDIO_ERR_READWRITE;
	}
	return data != (sdioif->response[0] & 0xFF);
}

/* Write given data to register, and read back the register into data */
int SDIO_WriteRead_Direct(LPC_SDMMC_T *pSDMMC, uint32 func, uint32 addr, uint32 *data)
{
	int ret;

	ret = SDIO_Send_Command(pSDMMC, CMD52, (func << 28) | (1 << 27) | (addr << 9) | ((*data) & 0xFF) | (1UL << 31));
	if (ret) return ret;

	/* Check response for errors */
	if(sdioif->response[0] & 0xcb00){
						/* COM_CRC_ERROR */
						/* ILLEGAL_CRC_ERROR */
						/* ERROR */
						/* RFU FUNCTION_NUMBER */
						/* OUT_OF_RANGE */
		/* Response flag error */
		return SDIO_ERR_READWRITE;
	}
	*data = sdioif->response[0] & 0xFF;
	return 0;
}

/* Read a register from the register address space of the CARD */
int SDIO_Read_Direct(LPC_SDMMC_T *pSDMMC, uint32 func, uint32 addr, uint32 *data)
{
	int ret;
	ret = SDIO_Send_Command(pSDMMC, CMD52, ((func & 7) << 28) | ((addr & 0x1FFFF) << 9));
	if (ret) return ret;

	/* Check response for errors */
	if(sdioif->response[0] & 0xcb00){
						/* COM_CRC_ERROR */
						/* ILLEGAL_CRC_ERROR */
						/* ERROR */
						/* RFU FUNCTION_NUMBER */
						/* OUT_OF_RANGE */
		/* Response flag error */
		return SDIO_ERR_READWRITE;
	}
	*data = sdioif->response[0] & 0xFF;
	return 0;
}

/* Set up the wait and wake call-back functions */
void SDIO_Setup_Callback(LPC_SDMMC_T *pSDMMC,
	void (*wake_evt)(LPC_SDMMC_T *pSDMMC, uint32 event, uint32 arg),
	uint32 (*wait_evt)(LPC_SDMMC_T *pSDMMC, uint32 event, uint32 arg))
{
	sdioif->wake_evt = wake_evt;
	sdioif->wait_evt = wait_evt;
}

/* Send and SD Command to the SDIO Card */
uint32 SDIO_Send_Command(LPC_SDMMC_T *pSDMMC, uint32 cmd, uint32 arg)
{
	uint32 ret, ival;
	uint32 imsk = pSDMMC->INTMASK;
	// ret = sdioif->wait_evt(pSDMMC, SDIO_START_COMMAND, (cmd & 0x3F));
	// ival = SDIO_CMD_INT_MSK & ~ret;
	ival = SDIO_CMD_INT_MSK;

	/* Set data interrupts for data commands */
	if (cmd & SDIO_CMD_DATA) {
		ival |= SDIO_DATA_INT_MSK;
		imsk |= SDIO_DATA_INT_MSK;
	}

	Chip_SDIF_SetIntMask(pSDMMC, ival);
	Chip_SDIF_SendCmd(pSDMMC, cmd, arg);
	// ret = sdioif->wait_evt(pSDMMC, SDIO_WAIT_COMMAND, 0);
	wait_for_sdio_irq(pSDMMC);
	pSDMMC->RINTSTS = 0xFFFFFFFF;	//clear interrupt
	// if (!ret && (cmd & SDIO_CMD_RESP_R1)) {
	// 	Chip_SDIF_GetResponse(pSDMMC, &sdioif->response[0]);
	// }

	Chip_SDIF_GetResponse(pSDMMC, &sdioif->response[0]);
	Chip_SDIF_SetIntMask(pSDMMC, imsk);
	return ret;
}


/* SDIO Card interrupt handler */
void SDIO_Handler(LPC_SDMMC_T *pSDMMC)
{
	uint32 status = pSDMMC->MINTSTS;
	uint32 iclr = 0;

	/* Card Detected */
	if (status & 1) {
		sdioif->wake_evt(pSDMMC, SDIO_CARD_DETECT, 0);
		iclr = 1;
	}

	/* Command event error */
	if (status & (SDIO_CMD_INT_MSK & ~4)) {
		sdioif->wake_evt(pSDMMC, SDIO_CMD_ERR, (status & (SDIO_CMD_INT_MSK & ~4)));
		iclr |= status & SDIO_CMD_INT_MSK;
	} else if (status & 4) {
		/* Command event done */
		sdioif->wake_evt(pSDMMC, SDIO_CMD_DONE, status);
		iclr |= status & SDIO_CMD_INT_MSK;
	}

	/* Command event error */
	if (status & (SDIO_DATA_INT_MSK & ~8)) {
		sdioif->wake_evt(pSDMMC, SDIO_DATA_ERR,  status & (SDIO_DATA_INT_MSK & ~8));
		iclr |= (status & SDIO_DATA_INT_MSK) | (3 << 4);
	} else if (status & 8) {
		/* Command event done */
		sdioif->wake_evt(pSDMMC, SDIO_DATA_DONE, status);
		iclr |= (status & SDIO_DATA_INT_MSK) | (3 << 4);
	}

	/* Handle Card interrupt */
	if (status & SDIO_CARD_INT_MSK) {
		sdioif->wake_evt(pSDMMC, SDIO_CARD_INT, 0);
		iclr |= status & SDIO_CARD_INT_MSK;
	}

	/* Clear the interrupts */
	pSDMMC->RINTSTS = iclr;
}






