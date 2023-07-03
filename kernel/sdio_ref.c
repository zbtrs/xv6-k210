/*
 * @brief Blinky example using sysTick
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
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

// #include "board.h"
// #include <stdio.h>
// #include "stopwatch.h"
#include "include/sdio_18xx_43xx.h"
#include "include/sdif_18xx_43xx.h"
#include "include/sdmmc.h"
#include "include/types.h"
#include "include/printf.h"
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define LPC_SDMMC_BASE            0x16020000
#define LPC_SDMMC                 ((LPC_SDMMC_T            *) LPC_SDMMC_BASE)

#define TICKRATE_HZ1 (1000)	/* 10 ticks per second */

void SDIO_IRQHandler(void);

static volatile uint32 ms_cnt;
/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
/** @brief	SDIO Driver states */
enum SDIO_STATE
{
	SDIO_STATE_IDLE,      /**! SDIO Driver IDLE */
	SDIO_STATE_CMD_WAIT,  /**! SDIO Driver waiting for CMD complete */
	SDIO_STATE_CMD_DONE,  /**! SDIO Command completed successfully */
	SDIO_STATE_CMD_ERR,   /**! SDIO Command transfer error */
	SDIO_STATE_DATA_WAIT,  /**! SDIO Driver waiting for data complete */
	SDIO_STATE_DATA_DONE,  /**! SDIO data completed successfully */
	SDIO_STATE_DATA_ERR,   /**! SDIO data transfer error */
};

/*****************************************************************************
 * Private functions
 ****************************************************************************/
static struct sdio_state
{
	enum SDIO_STATE cstate;
	enum SDIO_STATE dstate;
	uint32 carg;
	uint32 darg;
}sstate;

static volatile uint32 card_int;




static uint32 SDIO_WaitEvent(LPC_SDMMC_T *pSDMMC, uint32 event, uint32 arg)
{
	uint32 ret = 0;
	printf("%d", arg);
	switch (event) {
		case SDIO_START_COMMAND:
			sstate.cstate = SDIO_STATE_CMD_WAIT;
			break;

		case SDIO_START_DATA:
			sstate.dstate = SDIO_STATE_DATA_WAIT;
			break;

		case SDIO_WAIT_COMMAND:
			while (*((volatile enum SDIO_STATE *) &sstate.cstate) == SDIO_STATE_CMD_WAIT) {
				// __WFI();
				wait_for_sdio_irq(pSDMMC);
				printf("WFI\n");
			}
			ret = sstate.carg;
			break;

		case SDIO_WAIT_DATA:
			while (*((volatile enum SDIO_STATE *) &sstate.dstate) == SDIO_STATE_DATA_WAIT) {
				// __WFI();
				wait_for_sdio_irq(pSDMMC);
				printf("WFI\n");
			}
			ret = sstate.darg;
			break;

		case SDIO_WAIT_DELAY:
		{
			// uint32 cntr = ms_cnt + arg;
			// while (cntr > ms_cnt) {
			// 	printf("cntr: %d ms_cnt: %d", cntr, ms_cnt);
			// }
			while (arg)
			{
				arg--;
			}
			
			break;
		}
		default:
			break;
	}
	return ret;
}

static void SDIO_WakeEvent(LPC_SDMMC_T *pSDMMC, uint32 event, uint32 arg)
{
	switch (event) {
		case SDIO_CARD_DETECT:
			/* Handle Card Detect here */
			break;

		/* SDIO Command transmitted successfully */
		case SDIO_CMD_DONE:
			sstate.cstate = SDIO_STATE_CMD_DONE;
			sstate.carg = 0;
			break;

		/* SDIO Command has errors 'arg' has more info */
		case SDIO_CMD_ERR:
			sstate.cstate = SDIO_STATE_CMD_ERR;
			sstate.carg = arg;
			break;

		/* Error in data transfer */
		case SDIO_DATA_ERR:
			sstate.dstate = SDIO_STATE_DATA_ERR;
			sstate.darg =  arg;
			break;

		/* Data transfer completed successfully */
		case SDIO_DATA_DONE:
			sstate.dstate = SDIO_STATE_DATA_DONE;
			sstate.darg = 0;
			break;

		/* Interrupt from SDIO card function */
		case SDIO_CARD_INT:
			card_int = 1;
			break;

		default:
			break;
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	ms_cnt ++;
}

/**
 * @brief	SDIO controller interrupt handler
 * @return	Nothing
 */
void SDIO_IRQHandler(void)
{
	SDIO_Handler(LPC_SDMMC);
}

int GainSpan_GetLen(uint16 *len)
{
	int ret;
	uint32 val;

	/* Read length from Card */
	ret = SDIO_Read_Direct(LPC_SDMMC, 1, 0x1C, &val);
	if (ret) return ret;

	*len = val & 0xFF;
	/* Read length from Card */
	ret = SDIO_Read_Direct(LPC_SDMMC, 1, 0x1D, &val);
	if(ret) return ret;

	*len += val << 8;
	return 0;
}

int GainSpan_Enable(void)
{
	return SDIO_Write_Direct(LPC_SDMMC, 0, 0x2, 0x2);
}

// int UART_ReadLn(char *buf, int len)
// {
// 	int idx = 0;
// 	while (!card_int)
// 	{
// 		int ch = Board_UARTGetChar();
// 		if (len == idx) return idx;
// 		if (ch == EOF || ch == '\n') continue;
// 		if (ch == '\b') {
// 			if (idx) idx --;
// 			continue;
// 		}
// 		buf[idx++] = ch;
// 		if (ch == '\r' && idx != len){
// 			buf[idx++] = '\n';
// 			return idx;
// 		}
// 	}
// 	card_int = 0;
// 	return 0;
// }

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
int sdref_test(void)
{
	int ret;
	static uint8 dat[512];
	uint32 val;
	uint16 len;
	uint32 rca;
	uint16 fifo_depth;
	// SystemCoreClockUpdate();
	// Board_Init();

	/*  SD/MMC initialization */
	// Board_SDMMC_Init();
	printf("arrive 0\n");
	/* The SDIO driver needs to know the SDIO clock rate */
	Chip_SDIF_Init(LPC_SDMMC);
	printf("arrive 1\n");
	// NVIC_EnableIRQ(SDIO_IRQn);

	/* Wait for Card to be detected */
	while (Chip_SDIF_CardNDetect(LPC_SDMMC)) {}
	printf("arrive 2\n");
	/* Enable and setup SysTick Timer at a periodic rate */
	// SysTick_Config(SystemCoreClock / TICKRATE_HZ1);
	// Chip_Clock_SetDivider(CLK_IDIV_D, CLKIN_MAINPLL, 2);
	// Chip_Clock_SetBaseClock(CLK_BASE_SDIO, CLKIN_IDIVD, true, false);

	SDIO_Setup_Callback(LPC_SDMMC, SDIO_WakeEvent, SDIO_WaitEvent);
	printf("arrive 3\n");
	/* FIXME: Gainspan card works at (20MHz) frequency; needs investigation
	 * All other cards should use 400000 (400 KHz)
	 */

	// ret = SDIO_Card_Init(LPC_SDMMC, 20000000);
	printf("arrive 4\n");
	rca = SD_Card_Init(LPC_SDMMC, 400000);
	// if (ret != 0) {
	// 	printf("DBG: SDIO Card init error: 0x%X, %d\r\n", ret, ret);
	// }

	// if (!GainSpan_Enable()) {
	// 	printf("DBG: Gainspan Card IO Enabled\r\n");
	// }

	/* Set the Block size of the card */
	// if (!SDIO_Card_SetBlockSize(LPC_SDMMC, 1, 512)) {
	// 	printf("DBG: Block size set to 512\r\n");
	// }
	SD_Card_SetBlockSize(LPC_SDMMC, 512, rca);

	LPC_SDMMC->TMOUT = 0xffffff64;	//response time out = 0x64, ignore data time out

	// LPC_SDMMC->DEBNCE = 	//ignore

	printf("FIFOTH: %p\n", LPC_SDMMC->FIFOTH);
	
	uint32 fifoth_t = LPC_SDMMC->FIFOTH;

	fifo_depth = ((fifoth_t & 0x0fff0000) >> 16) + 1;

	LPC_SDMMC->FIFOTH = ((fifoth_t & 0xf0000000) | ((fifo_depth / 2 - 1) << 16) | (fifo_depth / 2));

	printf("FIFOTH: %p\n", LPC_SDMMC->FIFOTH);

	printf("HCON: %p\n", LPC_SDMMC->HCON);

	/* Enable the SDIO Card Interrupt */
	// if (!SDIO_Card_EnableInt(LPC_SDMMC, 1)) {
	// 	printf("DBG: Enabled interrupt for function 1\r\n");
	// }
	
	printf("Card interface enabled use AT commands!\r\n");

	// while (1) {

	// 	// len = UART_ReadLn((char *)dat, sizeof(dat));
	// 	len = 10;
	// 	for (int i = 0; i < len; i++)
	// 	{
	// 		dat[i] = i + '0';
	// 	}
		
	// 	if (len > 0) {
	// 		ret = SDIO_Card_WriteData(LPC_SDMMC, 1, 0x00, &dat[0], len, SDIO_MODE_BUFFER);
	// 		printf("arrive 5\n");
	// 		if (ret) /* Error writing data */
	// 			printf("DBG: Unable to write to SDIO card\r\n");
	// 		continue; /* Continue and wait for interrupt or data from UART */
	// 	}

	// 	/* Clear the interrupt */
	// 	ret = SDIO_Read_Direct(LPC_SDMMC, 1, 0x04, &val);
	// 	printf("arrive 6\n");
	// 	if (ret)
	// 		printf("DBG: Error clearing interrupt 0x%X\r\n", ret);

	// 	ret = GainSpan_GetLen(&len);
	// 	printf("arrive 7\n");
	// 	if (ret) /* Error getting length */
	// 		continue;

	// 	ret = SDIO_Card_ReadData(LPC_SDMMC, 1, &dat[0], 0x00, len, SDIO_MODE_BUFFER);
	// 	printf("arrive 8\n");
	// 	if (!ret) {
	// 		dat[len] = 0;
	// 		printf("%s", dat);
	// 	} else {
	// 		printf("DBG: SDIO Card RD error: 0x%X, %d\r\n", ret, ret);
	// 	}
	// }

	LPC_SDMMC->BLKSIZ = 512;
	LPC_SDMMC->BYTCNT = 512;
	
	
	
	
	
	SDIO_Send_Command(LPC_SDMMC, CMD24, 1);
	printf("response: %p\n", LPC_SDMMC->RESP0);

	
	int tt = 1;
	while (LPC_SDMMC->RINTSTS & 0x10)
	{
		*(uint32 *)(LPC_SDMMC_BASE + 0x200) = tt;
		tt++;
		// printf("rintst: %p\n", LPC_SDMMC->RINTSTS);
		// printf("data %d: %d\n", i, temp_data);
		for (int j = 0; j < 100000; j++)
		{
			/* code */
		}
		SDIO_IRQHandler();
	}
	
	
	printf("rintst: %p\n", LPC_SDMMC->RINTSTS);
	LPC_SDMMC->BLKSIZ = 512;
	LPC_SDMMC->BYTCNT = 512;
	SDIO_Send_Command(LPC_SDMMC, CMD17, 1);
	printf("response: %p\n", LPC_SDMMC->RESP0);
	uint32 temp_data;
	for (int i = 0; i < 16; i++)
	{
		// wait_for_read_irq(LPC_SDMMC);
		temp_data = *(uint32 *)(LPC_SDMMC_BASE + 0x200);
		printf("rintst: %p\n", LPC_SDMMC->RINTSTS);
		printf("data %d: %d\n", i, temp_data);
		for (int j = 0; j < 100000; j++)
		{
			/* code */
		}
	}

	return 0;
}






