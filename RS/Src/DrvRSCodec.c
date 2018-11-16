/***************************************************************************
 * Copyright (c) 2012 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *	DrvRSCodec.c
 * DESCRIPTION
 *	The library for RS.
 * FUNCTIONS
 *	None
 **************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "DrvRSCodec.h"
#include "w55fa92_edma.h"

/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
INT32 RS_TX_edma_ch = -1;
INT32 RS_RX_edma_ch = -1;
BOOL volatile g_bRSIntFlag = FALSE;	// interrupt flag in interrupt handler


/*-----------------------------------------------------------------------------
 * Interrupt handler for RS interrupt
 * OUTPUT:
 *	g_bRSIntFlag:	TRUE indicate RSCodec is finish
 *---------------------------------------------------------------------------*/
void RS_Int_Handler()
{
	UINT32 cntrl;

	cntrl = inp32(REG_RSC_CNTRL);

	if ((cntrl & RS_IE) && (cntrl & RS_IF)) {
		g_bRSIntFlag = TRUE;
		// clear interrupt flag
		outp32(REG_RSC_CNTRL, cntrl);
	}
}

/*-----------------------------------------------------------------------------
 * Clear interrupt flag in RS register
 *---------------------------------------------------------------------------*/
void RS_Clear_Int_Flag()
{
	g_bRSIntFlag = FALSE;
	// clear interrupt flag
	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | RS_IF);
}

/*-----------------------------------------------------------------------------
 * Enable RS interrupt feature in RS
 *---------------------------------------------------------------------------*/
void RS_Enable_Int(void)
{
	// clear all RS interrupt flags
	RS_Clear_Int_Flag();
	// enable RS interrupt
	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | RS_IE);
}

/*-----------------------------------------------------------------------------
 * Disable RS interrupt feature in RS
 *---------------------------------------------------------------------------*/
void RS_Disable_Int(void)
{
 	// clear all RS interrupt flags
	RS_Clear_Int_Flag();
	// disable RS interrupt
	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) & ~RS_IE);
}

void RS_PDMA_Free(void)
{
	if (RS_TX_edma_ch >= 0) {
		EDMA_Free(RS_TX_edma_ch);
		RS_TX_edma_ch = -1;
	}
	if (RS_RX_edma_ch >= 0) {
		EDMA_Free(RS_RX_edma_ch);
		RS_RX_edma_ch = -1;
	}
}

void RS_PDMA_Callback(UINT32 u32WrapStatus)
{
	RS_PDMA_Free();
	//DBG_PRINTF("RS_PDMA_Callback\n");
	return;
}

/*-----------------------------------------------------------------------------
 * Do PDMA settings for RS Codec
 * INPUT:
 *	UINT8*	inputBuf	// pointer to source buffer address
 *	UINT8*	outputBuf	// pointer to destination buffer address
 *	UINT32	blockNum	// block number of input data
 *	INT8	isDecrypt	// RS mode: encrypt or decrypt
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 RS_PDMA_Request(UINT8* inputBuf, UINT8* outputBuf, UINT32 blockNum, UINT8 isDecrypt)
{
	DBG_PRINTF("%s: inputBuf=0x%08X, outputBuf=0x%08X, blockNum=%d, isDecrypt=%d\n", 
			__func__, inputBuf, outputBuf, blockNum, isDecrypt);
	// for TX setting
	RS_TX_edma_ch = PDMA_FindandRequest();
	while (RS_TX_edma_ch < 0) {
		RS_TX_edma_ch = PDMA_FindandRequest();
	}
	EDMA_SetAPB(RS_TX_edma_ch, eDRVEDMA_RS_CODEC, eDRVEDMA_WRITE_APB, eDRVEDMA_WIDTH_32BITS);
	EDMA_SetupHandlers(RS_TX_edma_ch, eDRVEDMA_BLKD, NULL, NULL);
	EDMA_SetDirection(RS_TX_edma_ch, eDRVEDMA_DIRECTION_INCREMENTED, eDRVEDMA_DIRECTION_FIXED);
	if (isDecrypt)
		EDMA_SetupSingle(RS_TX_edma_ch, (UINT32)inputBuf, REG_RSC_WBUF, blockNum*RS_DECRYPT_BLOCK_SIZE);
	else
		EDMA_SetupSingle(RS_TX_edma_ch, (UINT32)inputBuf, REG_RSC_WBUF, blockNum*RS_ENCRYPT_BLOCK_SIZE);
	EDMA_Trigger(RS_TX_edma_ch);

	// for RX setting
	RS_RX_edma_ch = PDMA_FindandRequest();
	while (RS_RX_edma_ch < 0) {
		RS_RX_edma_ch = PDMA_FindandRequest();
	}
	EDMA_SetAPB(RS_RX_edma_ch, eDRVEDMA_RS_CODEC, eDRVEDMA_READ_APB, eDRVEDMA_WIDTH_32BITS);
	EDMA_SetupHandlers(RS_RX_edma_ch, eDRVEDMA_BLKD, RS_PDMA_Callback, NULL);
	EDMA_SetDirection(RS_RX_edma_ch, eDRVEDMA_DIRECTION_FIXED, eDRVEDMA_DIRECTION_INCREMENTED);
	if (isDecrypt)
		EDMA_SetupSingle(RS_RX_edma_ch, REG_RSC_RBUF, (UINT32)outputBuf, blockNum*RS_ENCRYPT_BLOCK_SIZE);
	else
		EDMA_SetupSingle(RS_RX_edma_ch, REG_RSC_RBUF, (UINT32)outputBuf, blockNum*RS_DECRYPT_BLOCK_SIZE);
	EDMA_Trigger(RS_RX_edma_ch);
	DBG_PRINTF("RS_TX_edma_ch=%d, RS_RX_edma_ch=%d\n", RS_TX_edma_ch, RS_RX_edma_ch);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Check the RS decode status
 * RETURN:
 *      Successful:	decode successful
 *      error code:	with decode error
 *---------------------------------------------------------------------------*/
INT32 RS_Check_Decode_Status()
{
	INT32 i, decStatus, errNum, regOffset, numShift, result;

	result = Successful;
	decStatus = inp32(REG_RSC_STATUS);
	// check RS decode status
	if (decStatus) {
		for (i = 0; i < 32; i++) {
			if (decStatus & (1 << i)) {
				sysprintf("ERROR in %s: RS block %d decode fail !!\n", __func__, i);
			} else {
				regOffset = (i/8)*4;
				numShift = (i%8)*4;
				errNum = (inp32(REG_RSC_ERRNUM0 + regOffset) >> numShift) & 0xF;
				if (errNum > 0)
					DBG_PRINTF("RS block %d correct %d errors\n", i, errNum);
			}
		}
		result = RSC_ERR_DEC_ERROR;
	} else if (inp32(REG_RSC_ERRNUM0) | inp32(REG_RSC_ERRNUM1) | inp32(REG_RSC_ERRNUM2) | inp32(REG_RSC_ERRNUM3)) {
		for (i = 0; i < 32; i++) {
			regOffset = (i/8)*4;
			numShift = (i%8)*4;
			errNum = (inp32(REG_RSC_ERRNUM0 + regOffset) >> numShift) & 0xF;
			if (errNum > 0)
				DBG_PRINTF("RS block %d correct %d errors\n", i, errNum);
		}
	}

	return result;
}

/*-----------------------------------------------------------------------------
 * Do RS Codec settings and then trigger it
 * INPUT:
 *	UINT8*	inputBuf	// pointer to source buffer address
 *	UINT8*	outputBuf	// pointer to destination buffer address
 *	UINT32	blockNum	// block number of input data
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 RS_Codec_Trigger(UINT8* inputBuf, UINT8* outputBuf, UINT32 blockNum)
{
	DBG_PRINTF("%s: inputBuf=0x%08X, outputBuf=0x%08X, blockNum=%d\n", __func__, inputBuf, outputBuf, blockNum);
	// check source buffer
	if (inputBuf == NULL) {
		sysprintf("ERROR in %s: must specify input buffer !!\n", __func__);
		return RSC_ERR_DATA_BUF;
	}

	// check destination buffer
	if (outputBuf == NULL) {
		sysprintf("ERROR in %s: must specify output buffer !!\n", __func__);
		return RSC_ERR_DATA_BUF;
	}

	RS_PDMA_Request(inputBuf, outputBuf, blockNum, (inp32(REG_RSC_CNTRL) & DEC_MODE) ? 1 : 0);

	// begin to do RS operation
	outp32(REG_RSC_EDMA, inp32(REG_RSC_EDMA) | EDMA_GO);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Wait RS Codec finish
 * OUTPUT:
 *	Put the result of RS operation to outputBuf specified by RS_Codec_Trigger
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 RS_Codec_Wait(void)
{
	INT32 result;

	// wait RS done
	if (inp32(REG_RSC_CNTRL) & RS_IE) {
		// interrupt enable, wait RS interrupt
		while (!g_bRSIntFlag) ;
		g_bRSIntFlag = FALSE;
	} else {
		// interrupt disable, wait RS polling
		while (inp32(REG_RSC_EDMA) & EDMA_GO) ;
	}

	result = Successful;
	if (inp32(REG_RSC_CNTRL) & DEC_MODE) {
		result = RS_Check_Decode_Status();
		if (result != Successful) {
			// bus error happen
			sysprintf("ERROR in %s: RS decode error %d !!\n", __func__, result);
		}
	}

	return result;
}

/*-----------------------------------------------------------------------------
 * Encrypt plain text by FA92 RS Codec
 * INPUT:
 *	UINT8*	plainBuf	// pointer to plain text buffer
 *	UINT8*	cipherBuf	// pointer to cipher text buffer
 *	UINT32	dataLen		// length of data, MUST be divisible by 188
 *	INT8	isInterleave	// RS mode: interleave or deinterleave
 * OUTPUT:
 *	Put the cipher text to buffer that cipherBuf pointed
 * RETURN:
 *	value >= 0:	OK and return output data length
 *	value < 0:	FAIL and return error code
 *---------------------------------------------------------------------------*/
INT32 RS_Encrypt(UINT8* plainBuf, UINT8* cipherBuf, INT32 dataLen, UINT8 isInterleave)
{
	UINT8 zeroArray[RS_ENCRYPT_BLOCK_SIZE*11];
	INT32 encodeSize, result, retVal;

	// check data length
#if 1
	if (dataLen % RS_ENCRYPT_BLOCK_SIZE != 0) {
		DBG_PRINTF("WARNING in %s: input length is not divisible by %d. we will add zero padding bytes !!\n", 
			__func__, RS_ENCRYPT_BLOCK_SIZE);
	}
#else
	if (dataLen % RS_ENCRYPT_BLOCK_SIZE != 0) {
		sysprintf("ERROR in %s: input length MUST be divisible by %d. Wrong length %d !!\n", 
			__func__, RS_ENCRYPT_BLOCK_SIZE, dataLen);
		return RSC_ERR_DATA_LEN;
	}
#endif

	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | RS_EN);
	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) & ~DEC_MODE);
	if (isInterleave == 1) {
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | CI_EN);
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | CI_INIT);
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | CI_CLR);
		while (inp32(REG_RSC_CNTRL) & CI_CLR) ;
	} else {
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) & ~CI_EN);
	}

	// flush D-Cache
	sysFlushCache(D_CACHE);

	retVal = dataLen / RS_ENCRYPT_BLOCK_SIZE * RS_DECRYPT_BLOCK_SIZE;
	while (dataLen > 0) {
		if (dataLen >= RS_ENCRYPT_MAX_SIZE) {
			result = RS_Codec_Trigger(plainBuf, cipherBuf, RS_ENCRYPT_MAX_SIZE/RS_ENCRYPT_BLOCK_SIZE);
			if (result != Successful)
				break;
			result = RS_Codec_Wait();
			if (result != Successful)
				break;
			dataLen -= RS_ENCRYPT_MAX_SIZE;
			plainBuf += RS_ENCRYPT_MAX_SIZE;
			cipherBuf += RS_DECRYPT_MAX_SIZE;
		} else {
			// final round
			if (dataLen % RS_ENCRYPT_BLOCK_SIZE == 0) {
				encodeSize = dataLen;
				result = RS_Codec_Trigger(plainBuf, cipherBuf, encodeSize/RS_ENCRYPT_BLOCK_SIZE);
			} else if (dataLen > RS_ENCRYPT_BLOCK_SIZE) {
				encodeSize = dataLen - (dataLen % RS_ENCRYPT_BLOCK_SIZE);
				result = RS_Codec_Trigger(plainBuf, cipherBuf, encodeSize/RS_ENCRYPT_BLOCK_SIZE);
			} else {
				encodeSize = RS_ENCRYPT_BLOCK_SIZE;
				memset(zeroArray, 0x0, RS_ENCRYPT_BLOCK_SIZE);
				memcpy(zeroArray, plainBuf, dataLen);
				sysFlushCache(D_CACHE);
				result = RS_Codec_Trigger(zeroArray, cipherBuf, 1);
				retVal += RS_DECRYPT_BLOCK_SIZE;
			}
			if (result != Successful)
				break;
			result = RS_Codec_Wait();
			if (result != Successful)
				break;
			plainBuf += encodeSize;
			cipherBuf += encodeSize/RS_ENCRYPT_BLOCK_SIZE*RS_DECRYPT_BLOCK_SIZE;
			dataLen -= encodeSize;
		}
	}

	if ((result == Successful) && (isInterleave == 1)) {
		// drain the data in interleaver FIFO
		memset(zeroArray, 0x0, sizeof(zeroArray[RS_ENCRYPT_BLOCK_SIZE*11]));
		result = RS_Codec_Trigger(zeroArray, cipherBuf, 11);
		if (result == Successful)
			result = RS_Codec_Wait();
		if (result == Successful)
			retVal += 11 * RS_DECRYPT_BLOCK_SIZE;
	}

	// flush and invalidate D-Cache
	sysFlushCache(D_CACHE);
	sysFlushCache(I_CACHE);
	sysInvalidCache();

	if (result != Successful)
		retVal = result;

	return retVal;
}

/*-----------------------------------------------------------------------------
 * Decrypt cipher text by FA92 RS Codec
 * INPUT:
 *	UINT8*	cipherBuf	// pointer to cipher text buffer
 *	UINT8*	plainBuf	// pointer to plain text buffer
 *	UINT32	dataLen		// length of data, MUST be divisible by 204
 *	INT8	isInterleave	// RS mode: interleave or deinterleave
 * OUTPUT:
 *	Put the plain text to buffer that plainBuf pointed
 * RETURN:
 *	value >= 0:	OK and return output data length
 *	value < 0:	FAIL and return error code
 *---------------------------------------------------------------------------*/
INT32 RS_Decrypt(UINT8* cipherBuf, UINT8* plainBuf, UINT32 dataLen, UINT8 isInterleave)
{
	INT32 result, retVal;

	// check data length
	if (dataLen % RS_DECRYPT_BLOCK_SIZE != 0) {
		sysprintf("ERROR in %s: input length MUST be divisible by %d. Wrong length %d !!\n", 
		__func__, RS_DECRYPT_BLOCK_SIZE, dataLen);
		return RSC_ERR_DATA_LEN;
	}

	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | RS_EN);
	outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | DEC_MODE);
	if (isInterleave == 1) {
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | CI_EN);
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | CI_INIT);
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) | CI_CLR);
		while (inp32(REG_RSC_CNTRL) & CI_CLR) ;
	} else {
		outp32(REG_RSC_CNTRL, inp32(REG_RSC_CNTRL) & ~CI_EN);
	}

	// flush D-Cache
	sysFlushCache(D_CACHE);

	if (isInterleave == 1) {
		// discard the zero data in interleaver FIFO
		result = RS_Codec_Trigger(cipherBuf, plainBuf, 11);
		if (result == Successful)
			result = RS_Codec_Wait();
		if (result == Successful) {
			dataLen -= 11 * RS_DECRYPT_BLOCK_SIZE;
			cipherBuf += 11 * RS_DECRYPT_BLOCK_SIZE;
		}
		if (result != Successful)
			return result;
	}

	retVal = dataLen / RS_DECRYPT_BLOCK_SIZE * RS_ENCRYPT_BLOCK_SIZE;
	while (dataLen > 0) {
		if (dataLen > RS_DECRYPT_MAX_SIZE) {
			result = RS_Codec_Trigger(cipherBuf, plainBuf, RS_DECRYPT_MAX_SIZE/RS_DECRYPT_BLOCK_SIZE);
			if (result != Successful)
				break;
			result = RS_Codec_Wait();
			if (result != Successful)
				break;
			dataLen -= RS_DECRYPT_MAX_SIZE;
			cipherBuf += RS_DECRYPT_MAX_SIZE;
			plainBuf += RS_ENCRYPT_MAX_SIZE;
		} else {
			// final round
			result = RS_Codec_Trigger(cipherBuf, plainBuf, dataLen/RS_DECRYPT_BLOCK_SIZE);
			if (result != Successful)
				break;
			result = RS_Codec_Wait();
			if (result != Successful)
				break;
			dataLen = 0;
		}
	}

	// flush and invalidate D-Cache
	sysFlushCache(D_CACHE);
	sysFlushCache(I_CACHE);
	sysInvalidCache();

	if (result != Successful)
		retVal = result;

	return retVal;
}

/*-----------------------------------------------------------------------------
 * Initial RSCodec settings
 *---------------------------------------------------------------------------*/
INT32 RS_Open(void)
{
	// 1. Check I/O pins. If I/O pins are used by other IPs, return error code
	// 2. Enable IP¡¦s clock
	outp32(REG_APBCLK, inp32(REG_APBCLK) | RSC_CKE);
	// 3. Reset IP
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) | RSCRST);
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~RSCRST);
	// 4. Configure IP according to inputted arguments
	// 5. Enable IP I/O pins
	// 6. Return 0 to present success

	EDMA_Init();

	// initial RS interrupt: hook up ISR and enable interrupt
	sysInstallISR(IRQ_LEVEL_7, IRQ_RSC, (PVOID)RS_Int_Handler);
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_RSC);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Initial RSCodec settings
 *---------------------------------------------------------------------------*/
void RS_Close(void)
{
	// 1. Disable IP I/O pins
	// 2. Disable IP¡¦s clock
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~RSC_CKE);
}
