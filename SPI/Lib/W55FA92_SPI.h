/**************************************************************************//**
 * @file     W55FA92_SPI.h
 * @version  V3.00
 * @brief    N3292x series SPI driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#ifndef _SPILIB_H_
#define _SPILIB_H_

#define SPI_SET_CLOCK			0

#define SPI_8BIT	8
#define SPI_16BIT	16
#define SPI_32BIT	32

typedef struct _spi_info_t
{
	INT32	nPort;
	BOOL	bIsSlaveMode;
	BOOL	bIsClockIdleHigh;
	BOOL	bIsLSBFirst;
	BOOL	bIsAutoSelect;
	BOOL	bIsActiveLow;
	BOOL	bIsTxNegative;
	BOOL	bIsLevelTrigger;
} SPI_INFO_T;

typedef enum
{
	eDRVSPI_DISABLE =0,
	eDRVSPI_ENABLE
}E_DRVSPI_OPERATION;

typedef enum
{
	eDRVSPI_DIRECTION_OUTPUT = 0,
	eDRVSPI_DIRECTION_INPUT = 0x1000000	
}E_DRVSPI_DIRECTION_SELECT;

typedef enum
{
	eDRVSPI_1BITS = 0,
	eDRVSPI_2BITS = 0x100,
	eDRVSPI_4BITS = 0x200
}E_DRVSPI_BITS_MODE_SELECT;

typedef VOID (*PFN_DRVSPI_CALLBACK)(void);

/* extern function */
VOID spiIoctl(INT32 spiPort, INT32 spiFeature, INT32 spicArg0, INT32 spicArg1);
INT  spiOpen(SPI_INFO_T *pInfo);
INT32 spiClose(UINT8 u8Port);
INT  spiRead(INT port, INT RxBitLen, INT len, CHAR *pDst);
INT  spiWrite(INT port, INT TxBitLen, INT len, CHAR *pSrc);
INT  spiEnable(INT32 spiPort);
INT  spiDisable(INT32 spiPort);
INT spiSSEnable(UINT32 spiPort, UINT32 SSPin, UINT32 ClockMode);
INT spiSSDisable(UINT32 spiPort, UINT32 SSPin);
INT spiTransfer(UINT32 port, UINT32 TxBitLen, UINT32 len, PUINT8 RxBuf , PUINT8 TxBuf);

#ifdef __FreeRTOS__
INT  spiRtosInit(INT32 spiPort);
/* for FreeRTOS SPI Flash */
INT  spiFlashInit(UINT32 spiPort, UINT32 SSPin);
INT  spiFlashReset(UINT32 spiPort, UINT32 SSPin);
INT  spiFlashEraseSector(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 secCount);
INT  spiFlashEraseBlock(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 blockCount);
INT  spiFlashEraseAll(UINT32 spiPort, UINT32 SSPin);
INT  spiFlashWrite(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf);
INT  spiFlashRead(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf);
INT spiFlashQuadWrite(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf);
INT spiFlashFastReadQuad(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf);
INT spiEONFlashQuadWrite(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf);
INT spiEONFlashFastReadQuad(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf);
INT  spiFlashEnter4ByteMode(UINT32 spiPort, UINT32 SSPin);
INT  spiFlashExit4ByteMode(UINT32 spiPort, UINT32 SSPin);
#else
/* for SPI Flash */
INT  spiFlashInit(void);
INT  spiFlashReset(void);
INT  spiFlashEraseSector(UINT32 addr, UINT32 secCount);
INT  spiFlashEraseBlock(UINT32 addr, UINT32 blockCount);
INT  spiFlashEraseAll(void);
INT  spiFlashWrite(UINT32 addr, UINT32 len, UINT8 *buf);
INT  spiFlashRead(UINT32 addr, UINT32 len, UINT8 *buf);
INT spiFlashQuadWrite(UINT32 addr, UINT32 len, UINT8 *buf);
INT spiFlashFastReadQuad(UINT32 addr, UINT32 len, UINT8 *buf);
INT spiEONFlashQuadWrite(UINT32 addr, UINT32 len, UINT8 *buf);
INT spiEONFlashFastReadQuad(UINT32 addr, UINT32 len, UINT8 *buf);
INT  spiFlashEnter4ByteMode(void);
INT  spiFlashExit4ByteMode(void);
#endif

/* internal function */
int spiActive(int port);
int spiTxLen(int port, int count, int bitLen);

void spiSetClock(int port, int clock_by_MHz, int output_by_kHz);
VOID spiSetByteEndin(UINT8 u8Port, E_DRVSPI_OPERATION eOP);

BOOL spiIsBusy(UINT8 u8Port);
VOID spiEnableInt(UINT8 u8Port);
VOID spiDisableInt(UINT8 u8Port);
VOID spi0IRQHandler(void);
VOID spi1IRQHandler(void);
ERRCODE
spiInstallCallBack(
	UINT8 u8Port,
	PFN_DRVSPI_CALLBACK pfncallback,
	PFN_DRVSPI_CALLBACK *pfnOldcallback
);

#ifdef __FreeRTOS__
int usiStatusWrite1(UINT32 spiPort, UINT32 SSPin, UINT8 data0, UINT8 data1);
int usiStatusWrite2(UINT32 spiPort, UINT32 SSPin, UINT8 data0);
int usiStatusRead(UINT32 spiPort, UINT32 SSPin, UINT8 cmd, PUINT8 data);
#else
int usiStatusWrite1(UINT8 data0, UINT8 data1);
int usiStatusWrite2(UINT8 data0);
int usiStatusRead(UINT8 cmd, PUINT8 data);
#endif

#endif
