/***************************************************************************
 * Copyright (c) 2012 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *	DrvCRC.c
 * DESCRIPTION
 *	The library for CRC.
 * FUNCTIONS
 *	None
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "DrvCRC.h"
#include "w55fa92_edma.h"

/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
S_CRC_CHANNEL_INFO g_CrcChInfo[CRC_CH_NUM];
UINT32 CRC_Order[] = {16, 8, 16, 32};
INT32 g_VdmaCh = -1;
BOOL bIsCRCInit = FALSE;
BOOL g_bCrcInitFlag = FALSE;
volatile BOOL g_bVdmaIntFlag = FALSE;

void CRC_VDMA_Free(void)
{
	EDMA_Free(g_VdmaCh);
	g_bVdmaIntFlag = TRUE;
}

void CRC_VDMA_Callback(UINT32 u32WrapStatus)
{
	CRC_VDMA_Free();
	//DBG_PRINTF("CRC_VDMA_Callback\n");
	return;
}

/*-----------------------------------------------------------------------------
 * Do VDMA settings for CRC
 * INPUT:
 *	UINT8*	pSrcBuf		// pointer to source buffer address
 *	UINT32	uDataLen	// input data length
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 CRC_VDMA_Trigger(UINT8* pSrcBuf, UINT32 pDestBuf, UINT32 uDataLen)
{
	g_VdmaCh = VDMA_FindandRequest();
	while (g_VdmaCh < 0) {
		g_VdmaCh = VDMA_FindandRequest();
	}

	EDMA_SetupHandlers(g_VdmaCh, eDRVEDMA_BLKD_FLAG, CRC_VDMA_Callback, NULL);
	EDMA_SetDirection(g_VdmaCh, eDRVEDMA_DIRECTION_INCREMENTED, eDRVEDMA_DIRECTION_INCREMENTED);
	EDMA_SetupSingle(g_VdmaCh, (UINT32)pSrcBuf, pDestBuf, uDataLen);
	EDMA_Trigger(g_VdmaCh);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Wait CRC VDMA finish
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 CRC_VDMA_Wait(void)
{
	// wait CRC VDMA done
	while (!g_bVdmaIntFlag) ;
	g_bVdmaIntFlag = FALSE;

	return Successful;
}

/*-----------------------------------------------------------------------------
 * CRC_EnableCH - enable/disable specified channel number
 * @echannel: CRC channel number
 * @eOP: enable or disable operation
 *---------------------------------------------------------------------------*/
void CRC_EnableCH(E_CRC_CHANNEL_INDEX eChannel, E_CRC_OPERATION eOP)
{
	UINT32 uCkeBit, uRstBit;

	DBG_PRINTF("CRC%d: %s\n", eChannel, __func__);

	if (eChannel < CRC_CH_NUM) {
		uCkeBit = (eChannel == E_CHANNEL_0) ? CRC_CKE : CRC1_CKE;
		uRstBit = (eChannel == E_CHANNEL_0) ? CRC_RST : CRC1_RST;

		if (eOP == E_CH_DISABLE) {
			// 1. Disable IP I/O pins
			// 2. Disable IP¡¦s clock
			outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & ~uCkeBit);
		} else {
			// 1. Check I/O pins. If I/O pins are used by other IPs, return error code
			// 2. Enable IP¡¦s clock
			outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | uCkeBit);
			// 3. Reset IP
			outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | uRstBit);
			outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~uRstBit);
			// 4. Configure IP according to inputted arguments
			// 5. Enable IP I/O pins
		}
	}
}

/*-----------------------------------------------------------------------------
 * CRC_Request - request/allocate specified channel number
 * @channel: CRC channel number
 *---------------------------------------------------------------------------*/
INT32 CRC_Request(INT32 channel)
{
	S_CRC_CHANNEL_INFO *pCrcInfo = &g_CrcChInfo[channel];

	DBG_PRINTF("CRC%d: %s\n", channel, __func__);

	if (channel >= MAX_CHANNEL_NUM) {
		sysprintf("%s Error: called for non-existed channel %d!\n", __func__, channel);
		return CRC_ERR_INVAL;
	}

	sysSetLocalInterrupt(DISABLE_IRQ);

	if (pCrcInfo->bInRequest)
		return CRC_ERR_STATUS;
	if (pCrcInfo->bInUse)
		return CRC_ERR_BUSY;

	memset((void*)pCrcInfo, 0x0, sizeof(pCrcInfo));
	pCrcInfo->bInRequest = TRUE;

	// Enable CRC channel
	CRC_EnableCH((E_CRC_CHANNEL_INDEX)channel, E_CH_ENABLE);

	sysSetLocalInterrupt(ENABLE_IRQ);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * CRC_Free - release previously acquired channel
 * @channel: CRC channel number
 *---------------------------------------------------------------------------*/
void CRC_Free(INT32 channel)
{	
	S_CRC_CHANNEL_INFO *pCrcInfo = &g_CrcChInfo[channel];

	DBG_PRINTF("CRC%d: %s\n", channel, __func__);

	sysSetLocalInterrupt(DISABLE_IRQ);

	if (!pCrcInfo->bInRequest) {
		sysprintf("%s Error: trying to free a un-requested channel %d!\n", __func__, channel);
		return;
	}

	if (pCrcInfo->bInUse)
		pCrcInfo->bInUse = FALSE;
	pCrcInfo->bInRequest = FALSE;

	// Disable CRC channel
	CRC_EnableCH((E_CRC_CHANNEL_INDEX)channel, E_CH_DISABLE);

	sysSetLocalInterrupt(ENABLE_IRQ);
}

/*-----------------------------------------------------------------------------
 * CRC_FindandRequest - find and request a free channels
 *
 * This function tries to find a free channel in the specified priority group
 *
 * Return value: If there is no free channel to allocate, CRC_ERR_NODEV is returned.
 *               On successful allocation channel is returned.
 *---------------------------------------------------------------------------*/
INT32 CRC_FindandRequest(void)
{
	INT32 i;

	for (i = 0; i < CRC_CH_NUM; i++)
		if (!CRC_Request(i))
			return i;

	sysprintf("%s Error: no free CRC channel is found!\n", __func__);

	return CRC_ERR_NODEV;
}

/*-----------------------------------------------------------------------------
 * Do CRC action
 * INPUT:
 *	UINT8*			pDataBuf	// pointer to source buffer address
 *	UINT32			uDataLen	// input data length
 *	S_CRC_DESCRIPT_SETTING*	psCRCDescript	// pointer to the structure CRC descript
 * RETURN:
 *	The CRC checksum
 *---------------------------------------------------------------------------*/
UINT32 CRC_Run(INT32 channel, UINT8 *pDataBuf, UINT32 uDataLen, S_CRC_DESCRIPT_SETTING *psCRCDescript)
{
	S_CRC_CHANNEL_INFO *pCrcInfo = &g_CrcChInfo[channel];
	UINT32 uOffset, uCRCCtrl, uCRCMask, uVDMALength, uInteger, i;
	UINT8 *pBufAddr, uCRCMode, uWriteLength, uRemainder;
	INT8 IsInput1sCOM, IsInputRVS, IsCRC1sCOM, IsCRCRVS;

	DBG_PRINTF("CRC%d: %s\n", channel, __func__);
	if (!pCrcInfo->bInRequest) {
		sysprintf("%s Error: CRC channel %d is not requested!\n", __func__, channel);
		return CRC_ERR_STATUS;
	}
	if (pCrcInfo->bInUse) {
		sysprintf("%s Error: CRC channel %d is busy!\n", __func__, channel);
		return CRC_ERR_BUSY;
	}
	pCrcInfo->bInUse = TRUE;

	uOffset = channel * 0x2000;
	uCRCMode = psCRCDescript->ePolyMode;
	IsInput1sCOM = psCRCDescript->eWdataCom;
	IsInputRVS = psCRCDescript->eWdataRvs;
	IsCRC1sCOM = psCRCDescript->eChecksumCom;
	IsCRCRVS = psCRCDescript->eChecksumRvs;
	uWriteLength = 1 << psCRCDescript->eWriteLength;

	// enable CRC
	outp32(REG_CRC_CTL + uOffset, CRCCEN);
	// clear default settings of CRC
	uCRCCtrl = inp32(REG_CRC_CTL + uOffset) &~ (CRC_MODE|CPU_WDLEN|CHECKSUM_COM|WDATA_COM|CHECKSUM_RVS|WDATA_RVS);
	uCRCCtrl |= (uCRCMode<<30)|(IsCRC1sCOM<<27)|(IsInput1sCOM<<26)|(IsCRCRVS<<25)|(IsInputRVS<<24);
	outp32(REG_CRC_CTL + uOffset, uCRCCtrl);
	outp32(REG_CRC_SEED + uOffset, psCRCDescript->uSeed);
	// reset CRC
	outp32(REG_CRC_CTL + uOffset, uCRCCtrl | CRC_SW_RST);

	pBufAddr = pDataBuf;
	if (psCRCDescript->eTransferMode) {
		// flush and invalidate the D-Cache
		sysFlushCache(D_CACHE);
		// CRC must set word data width for VDMA mode
		outp32(REG_CRC_CTL + uOffset, uCRCCtrl | (E_LENGTH_WORD << 28));
		while (uDataLen >= CRC_VDMA_MAX_SIZE) {
			CRC_VDMA_Trigger(pBufAddr, REG_DMA_WDATA + uOffset, CRC_VDMA_MAX_SIZE);
			CRC_VDMA_Wait();
			uDataLen -= CRC_VDMA_MAX_SIZE;
			pBufAddr += CRC_VDMA_MAX_SIZE;
		}

		uWriteLength = 1 << E_LENGTH_WORD;
		uRemainder = uDataLen % uWriteLength;
		uVDMALength = uDataLen - uRemainder;
		if (uVDMALength > 0) {
			CRC_VDMA_Trigger(pBufAddr, REG_DMA_WDATA + uOffset, uVDMALength);
			CRC_VDMA_Wait();
			uDataLen -= uVDMALength;
			pBufAddr += uVDMALength;
		}
	} else {
		uWriteLength = 1 << psCRCDescript->eWriteLength;
		uInteger = uDataLen / uWriteLength;
		uRemainder = uDataLen % uWriteLength;

		// CRC can set word data width as byte/half-word/word for PIO mode
		outp32(REG_CRC_CTL + uOffset, uCRCCtrl | (psCRCDescript->eWriteLength << 28));
		for (i = 0; i < uInteger; i++) {
			switch (uWriteLength) {
			case 1 :
				outp8(REG_CRC_WDATA + uOffset, *pBufAddr);
				break;
			case 2 :
				outp16(REG_CRC_WDATA + uOffset, *(UINT16 *)(pBufAddr));
				break;
			default :
				outp32(REG_CRC_WDATA + uOffset, *(UINT32 *)(pBufAddr));
				break;
			}
			uDataLen -= uWriteLength;
			pBufAddr += uWriteLength;
		}
	}

	// process the remain data
	if (uRemainder > 0) {
		uCRCCtrl = inp32(REG_CRC_CTL + uOffset) &~ CPU_WDLEN;
		outp32(REG_CRC_CTL + uOffset, uCRCCtrl | (E_LENGTH_BYTE << 28));
		while (uDataLen > 0) {
			outp8(REG_CRC_WDATA + uOffset, *pBufAddr);
			uDataLen--;
			pBufAddr++;
		}
	}
	uCRCMask = ((((UINT32)1 << (CRC_Order[uCRCMode] - 1)) - 1) << 1) | 1;

	pCrcInfo->bInUse = FALSE;

	return (inp32(REG_CRC_CHECKSUM + uOffset) & uCRCMask);
}

INT32 CRC_Init(void)
{
	INT32 i;

	DBG_PRINTF("%s\n", __func__);

	if (bIsCRCInit == FALSE) {
		bIsCRCInit = TRUE;
		for (i = 0; i < CRC_CH_NUM; i++) {
			g_CrcChInfo[i].bInRequest = FALSE;
			g_CrcChInfo[i].bInUse = FALSE;
		}
	}

	EDMA_Init();
	sysSetLocalInterrupt(ENABLE_IRQ);

	return Successful;
}

void CRC_Exit(void)
{
	DBG_PRINTF("%s\n", __func__);

	bIsCRCInit = TRUE;
}
