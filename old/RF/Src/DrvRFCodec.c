/***************************************************************************
 * Copyright (c) 2011 Nuvoton Technology. All rights reserved.
 *
 * FILENAME
 *	DrvRFCodec.c
 * DESCRIPTION
 *	The library for RF.
 * FUNCTIONS
 *	None
 **************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "DrvRFCodec.h"
#include "w55fa92_edma.h"

/*-----------------------------------------------------------------------------
 * Define Global Variables
 *---------------------------------------------------------------------------*/
INT32 RF_edma_in = -1;
INT32 RF_edma_out = -1;
BOOL volatile g_bRFIntFlag = FALSE;	// interrupt flag in interrupt handler


/*-----------------------------------------------------------------------------
 * Interrupt handler for RF interrupt
 * OUTPUT:
 *	g_bRFIntFlag:	TRUE indicate RFCodec is finish
 *---------------------------------------------------------------------------*/
void RF_Int_Handler()
{
	UINT32 cntrl;

	cntrl = inp32(REG_RFCODEC_CTL);

	if ((cntrl & RF_INT_EN) && (cntrl & RF_INT)) {
		g_bRFIntFlag = TRUE;
		// clear interrupt flag
		outp32(REG_RFCODEC_CTL, cntrl);
	}
}

/*-----------------------------------------------------------------------------
 * Clear interrupt flag in RF register
 *---------------------------------------------------------------------------*/
void RF_Clear_Int_Flag()
{
	g_bRFIntFlag = FALSE;
	// clear interrupt flag
	outp32(REG_RFCODEC_CTL, inp32(REG_RFCODEC_CTL) | RF_INT);
}

/*-----------------------------------------------------------------------------
 * Enable RF interrupt feature in RF
 *---------------------------------------------------------------------------*/
void RF_Enable_Int(void)
{
	// clear all RF interrupt flags
	RF_Clear_Int_Flag();
	// enable RF interrupt
	outp32(REG_RFCODEC_CTL, inp32(REG_RFCODEC_CTL) | RF_INT_EN);
}

/*-----------------------------------------------------------------------------
 * Disable RF interrupt feature in RF
 *---------------------------------------------------------------------------*/
void RF_Disable_Int(void)
{
 	// clear all RF interrupt flags
	RF_Clear_Int_Flag();
	// disable RF interrupt
	outp32(REG_RFCODEC_CTL, inp32(REG_RFCODEC_CTL) & ~RF_INT_EN);
}

void RF_PDMA_Free(void)
{
	if (RF_edma_in >= 0) {
		EDMA_Free(RF_edma_in);
		RF_edma_in = -1;
	}
	if (RF_edma_out >= 0) {
		EDMA_Free(RF_edma_out);
		RF_edma_out = -1;
	}
}

void RF_PDMA_Callback(UINT32 u32WrapStatus)
{
	RF_PDMA_Free();
//	DBG_PRINTF("RF_PDMA_Callback\n");
	return;
}

INT32 RF_Set_Puncture(E_RF_PNCTR_MODE ePnctrMod)
{
	if ((ePnctrMod >= E_PNCTR_1_2) && (ePnctrMod <= E_PNCTR_7_8)) {
		outp32(REG_RFCODEC_CTL, (inp32(REG_RFCODEC_CTL) & ~PNCTR_MOD) | (ePnctrMod << 4));
		return Successful;
	} else {
		sysprintf("ERROR in %s: wrong puncture mode !!\n", __func__);
		return RFC_ERR_PNCTR_MODE;
	}
}

E_RF_PNCTR_MODE RF_Get_Puncture(void)
{
	E_RF_PNCTR_MODE ePnctrMod;

	ePnctrMod = (E_RF_PNCTR_MODE)((inp32(REG_RFCODEC_CTL) & PNCTR_MOD) >> 4);

	return ePnctrMod;
}

INT32 RF_Get_Puncture_Number(UINT32* pnctrNum, UINT32* pnctrDen)
{
	E_RF_PNCTR_MODE ePnctrMod;

	ePnctrMod = (E_RF_PNCTR_MODE)((inp32(REG_RFCODEC_CTL) & PNCTR_MOD) >> 4);

	switch (ePnctrMod) {
	case E_PNCTR_1_2 :
		*pnctrNum = 1;
		*pnctrDen = 2;
		break;
	case E_PNCTR_2_3 :
		*pnctrNum = 2;
		*pnctrDen = 3;
		break;
	case E_PNCTR_3_4 :
		*pnctrNum = 3;
		*pnctrDen = 4;
		break;
	case E_PNCTR_5_6 :
		*pnctrNum = 5;
		*pnctrDen = 6;
		break;
	case E_PNCTR_7_8 :
		*pnctrNum = 7;
		*pnctrDen = 8;
		break;
	default :
		sysprintf("ERROR in %s: wrong puncture mode !!\n", __func__);
		return RFC_ERR_PNCTR_MODE;
	}

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Do PDMA settings for RF Codec
 * INPUT:
 *	UINT8*	inputBuf	// pointer to source buffer address
 *	UINT8*	outputBuf	// pointer to destination buffer address
 *	UINT32	inDataLen	// input data length
 *	UINT32	outDataLen	// output data length
 *	INT8	isDecrypt	// RF mode: encrypt or decrypt
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 RF_PDMA_Request(UINT8* inputBuf, UINT8* outputBuf, UINT32 inDataLen, UINT32 outDataLen, UINT8 isDecrypt)
{
	UINT32 inReg, outReg;

	RF_edma_in = PDMA_FindandRequest();
	while (RF_edma_in < 0) {
		RF_edma_in = PDMA_FindandRequest();
	}
	RF_edma_out = PDMA_FindandRequest();
	while (RF_edma_out < 0) {
		RF_edma_out = PDMA_FindandRequest();
	}
	if (isDecrypt) {
		inReg = REG_RFCODEC_INTRLV;
		outReg = REG_RFCODEC_DAT;
	} else {
		inReg = REG_RFCODEC_DAT;
		outReg = REG_RFCODEC_INTRLV;
	}

	// for input step
	EDMA_SetAPB(RF_edma_in, eDRVEDMA_RF_CODEC, eDRVEDMA_WRITE_APB, eDRVEDMA_WIDTH_8BITS);
	EDMA_SetupHandlers(RF_edma_in, eDRVEDMA_BLKD, NULL, NULL);
	EDMA_SetDirection(RF_edma_in, eDRVEDMA_DIRECTION_INCREMENTED, eDRVEDMA_DIRECTION_FIXED);
	EDMA_SetupSingle(RF_edma_in, (UINT32)inputBuf, inReg, inDataLen);
	EDMA_Trigger(RF_edma_in);

	// for output step
	EDMA_SetAPB(RF_edma_out, eDRVEDMA_RF_CODEC, eDRVEDMA_READ_APB, eDRVEDMA_WIDTH_8BITS);
	EDMA_SetupHandlers(RF_edma_out, eDRVEDMA_BLKD, RF_PDMA_Callback, NULL);
	EDMA_SetDirection(RF_edma_out, eDRVEDMA_DIRECTION_FIXED, eDRVEDMA_DIRECTION_INCREMENTED);
	EDMA_SetupSingle(RF_edma_out, outReg, (UINT32)outputBuf, outDataLen);
	EDMA_Trigger(RF_edma_out);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Do RF Codec settings and then trigger it
 * INPUT:
 *	UINT8*	inputBuf	// pointer to source buffer address
 *	UINT8*	outputBuf	// pointer to destination buffer address
 *	UINT32	inDataLen	// input data length
 *	UINT32	outDataLen	// output data length
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 RF_Codec_Trigger(UINT8* inputBuf, UINT8* outputBuf, UINT32 inDataLen, UINT32 outDataLen)
{
	//DBG_PRINTF("%s: inputBuf=%p, outputBuf=%p, inDataLen=%d, outDataLen=%d\n", __func__, inputBuf, outputBuf, inDataLen, outDataLen);
	// check source buffer
	if (inputBuf == NULL) {
		sysprintf("ERROR in %s: must specify input buffer !!\n", __func__);
		return RFC_ERR_DATA_BUF;
	}
	// check destination buffer
	if (outputBuf == NULL) {
		sysprintf("ERROR in %s: must specify output buffer !!\n", __func__);
		return RFC_ERR_DATA_BUF;
	}

	if (inp32(REG_RFCODEC_CTL) & RF_MODE) {
		// set data byte length for decode output
		outp32(REG_RFCODEC_CTL, (inp32(REG_RFCODEC_CTL) &~ DAT_LEN_BYTE) | (outDataLen << 16));
		RF_PDMA_Request(inputBuf, outputBuf, inDataLen, outDataLen,  1);
	} else {
		// set data byte length for encode input
		outp32(REG_RFCODEC_CTL, (inp32(REG_RFCODEC_CTL) &~ DAT_LEN_BYTE) | (inDataLen << 16));
		RF_PDMA_Request(inputBuf, outputBuf, inDataLen, outDataLen,  0);
	}

	// begin to do RF operation
	outp32(REG_RFCODEC_CTL, inp32(REG_RFCODEC_CTL) | RF_START);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Wait RF Codec finish
 * OUTPUT:
 *	Put the result of RF operation to outputBuf specified by RF_Codec_Trigger()
 * RETURN:
 *	Successful:	OK
 *	error code:	FAIL
 *---------------------------------------------------------------------------*/
INT32 RF_Codec_Wait(void)
{
	// wait RF done
	if (inp32(REG_RFCODEC_CTL) & RF_INT_EN) {
		// interrupt enable, wait RF interrupt
		while (!g_bRFIntFlag) ;
		g_bRFIntFlag = FALSE;
	} else {
		// interrupt disable, wait RF polling
		while (inp32(REG_RFCODEC_CTL) & RF_START) ;
	}

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Encrypt plain text by FA92 RF Codec
 * INPUT:
 *	UINT8*	plainBuf	// pointer to plain text buffer
 *	UINT8*	cipherBuf	// pointer to cipher text buffer
 *	UINT32	plainDataLen	// length of plain data
 * OUTPUT:
 *	Put the cipher text to buffer that cipherBuf pointed
 * RETURN:
 *	value >= 0:	OK and return output data length
 *	value < 0:	FAIL and return error code
 *---------------------------------------------------------------------------*/
INT32 RF_Encrypt(UINT8* plainBuf, UINT8* cipherBuf, INT32 plainDataLen)
{
	UINT8* pre_cipherBuf;
	UINT32 pnctrNum, pnctrDen, outDataLen;
	INT32 result, retVal;

	outp32(REG_RFCODEC_CTL, inp32(REG_RFCODEC_CTL) & ~RF_MODE);

	if (RF_Get_Puncture_Number(&pnctrNum, &pnctrDen) < 0)
		sysprintf("ERROR in %s: get RF puncture number fail !!\n", __func__);

	// flush D-Cache
	sysFlushCache(D_CACHE);

	pre_cipherBuf = cipherBuf;
	while (plainDataLen > 0) {
		if (plainDataLen >= RF_MAX_SIZE) {
			outDataLen = (((RF_MAX_SIZE+1)*pnctrDen+pnctrNum-1)/pnctrNum+23)/24*24;
			result = RF_Codec_Trigger(plainBuf, cipherBuf, RF_MAX_SIZE, outDataLen);
			if (result != Successful)
				break;
			result = RF_Codec_Wait();
			if (result != Successful)
				break;
			plainBuf += RF_MAX_SIZE;
			// skip padding 24 bytes
			//cipherBuf += (outDataLen-24);
			cipherBuf += outDataLen;
			plainDataLen -= RF_MAX_SIZE;
		} else {
			// final round
			outDataLen = (((plainDataLen+1)*pnctrDen+pnctrNum-1)/pnctrNum+23)/24*24;
			result = RF_Codec_Trigger(plainBuf, cipherBuf, plainDataLen, outDataLen);
			if (result != Successful)
				break;
			result = RF_Codec_Wait();
			if (result != Successful)
				break;
			plainBuf += plainDataLen;
			cipherBuf += outDataLen;
			plainDataLen -= plainDataLen;
		}
	}
	retVal = (UINT32)(cipherBuf - pre_cipherBuf);

	// flush and invalidate D-Cache
	sysFlushCache(D_CACHE);
	sysFlushCache(I_CACHE);
	sysInvalidCache();

	if (result != Successful)
		retVal = result;

	return retVal;
}

/*-----------------------------------------------------------------------------
 * Decrypt cipher text by FA92 RF Codec
 * INPUT:
 *	UINT8*	cipherBuf	// pointer to cipher text buffer
 *	UINT8*	plainBuf	// pointer to plain text buffer
 *	UINT32	plainDataLen	// length of plain data
 * OUTPUT:
 *	Put the plain text to buffer that plainBuf pointed
 * RETURN:
 *	value >= 0:	OK and return output data length
 *	value < 0:	FAIL and return error code
 *---------------------------------------------------------------------------*/
INT32 RF_Decrypt(UINT8* cipherBuf, UINT8* plainBuf, UINT32 plainDataLen)
{
	UINT32 pnctrNum, pnctrDen, inDataLen;
	INT32 result, retVal;

	outp32(REG_RFCODEC_CTL, inp32(REG_RFCODEC_CTL) | RF_MODE);

	if (RF_Get_Puncture_Number(&pnctrNum, &pnctrDen) < 0)
		sysprintf("ERROR in %s: get RF puncture number fail !!\n", __func__);

	// flush D-Cache
	sysFlushCache(D_CACHE);

	retVal = plainDataLen;
	while (plainDataLen > 0) {
		if (plainDataLen >= RF_MAX_SIZE) {
			inDataLen = (((RF_MAX_SIZE+1)*pnctrDen+pnctrNum-1)/pnctrNum+23)/24*24;
			result = RF_Codec_Trigger(cipherBuf, plainBuf, inDataLen, RF_MAX_SIZE);
			if (result != Successful)
				break;
			result = RF_Codec_Wait();
			if (result != Successful)
				break;
			// skip padding 24 bytes
			//cipherBuf += (inDataLen-24);
			cipherBuf += inDataLen;
			plainBuf += RF_MAX_SIZE;
			plainDataLen -= RF_MAX_SIZE;
		} else {
			inDataLen = (((plainDataLen+1)*pnctrDen+pnctrNum-1)/pnctrNum+23)/24*24;
			// final round
			result = RF_Codec_Trigger(cipherBuf, plainBuf, inDataLen, plainDataLen);
			if (result != Successful)
				break;
			result = RF_Codec_Wait();
			if (result != Successful)
				break;
			cipherBuf += inDataLen;
			plainBuf += plainDataLen;
			plainDataLen -= plainDataLen;
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
 * Initial RFCodec settings
 *---------------------------------------------------------------------------*/
INT32 RF_Open(void)
{
	// 1. Check I/O pins. If I/O pins are used by other IPs, return error code
	// 2. Enable IP¡¦s clock
	outp32(REG_APBCLK, inp32(REG_APBCLK) | CONVENC_CKE);
	// 3. Reset IP
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) | RFCRST);
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~RFCRST);
	// 4. Configure IP according to inputted arguments
	// 5. Enable IP I/O pins
	// 6. Return 0 to present success

	EDMA_Init();

	// initial RF interrupt: hook up ISR and enable interrupt
	sysInstallISR(IRQ_LEVEL_7, IRQ_VTB, (PVOID)RF_Int_Handler);
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_VTB);

	return Successful;
}

/*-----------------------------------------------------------------------------
 * Initial RFCodec settings
 *---------------------------------------------------------------------------*/
void RF_Close(void)
{
	// 1. Disable IP I/O pins
	// 2. Disable IP¡¦s clock
	outp32(REG_APBCLK, inp32(REG_APBCLK) & ~CONVENC_CKE);
}
