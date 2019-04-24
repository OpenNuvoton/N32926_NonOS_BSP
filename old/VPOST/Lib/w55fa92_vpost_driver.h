/***************************************************************************
 *                                                                         *
 * Copyright (c) 2007 - 2009 Nuvoton Technology Corp. All rights reserved.*
 *                                                                         *
 ***************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     w55fa92_vpost_driver.h
 *
 * VERSION
 *     0.1 
 *
 * DESCRIPTION
 *
 *
 *
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *
 *
 *     
 * HISTORY
 *     2009.03.16		Created by Shu-Ming Fan
 *
 *
 * REMARK
 *     None
 *
 *
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wblib.h"
#include "w55FA92_vpost.h"

extern volatile PFN_DRVVPOST_INT_CALLBACK g_pfnVpostCallback[4];
extern VOID vpostVAStartTrigger(void);
extern VOID vpostVAStopTrigger(void);
extern VOID vpostVAStartTrigger_MPUContinue(void);
extern VOID vpostVAStartTrigger_MPUSingle(void);
extern VOID vpostVAStopTriggerMPU(void);
extern BOOL vpostAllocVABuffer(PLCDFORMATEX plcdformatex,UINT32 nBytesPixel);
extern BOOL vpostAllocVABufferFromAP(UINT32 *pFramebuf);
extern BOOL vpostClearVABuffer(void);
extern BOOL vpostFreeVABuffer(void);
extern VOID vpostSetLCDEnable(BOOL bYUVBL, UINT8 ucVASrcType, BOOL bLCDRun);
extern VOID vpostSetLCDConfig(BOOL bLCDSynTv, UINT8 u8LCDDataSel, UINT8 u8LCDTYPE);
extern VOID vpostsetLCM_TimingType(E_DRVVPOST_TIMING_TYPE eTimingTpye);
extern VOID vpostSetLCM_TypeSelect(E_DRVVPOST_LCM_TYPE eType);
extern VOID vpostSetSerialSyncLCM_Interface(E_DRVVPOST_8BIT_SYNCLCM_INTERFACE eInterface);
extern VOID vpostSetSerialSyncLCM_ColorOrder(E_DRVVPOST_SERAIL_SYNCLCM_COLOR_ORDER eEvenLineOrder,E_DRVVPOST_SERAIL_SYNCLCM_COLOR_ORDER eOddLineOrder);
extern VOID vpostSetSerialSyncLCM_CCIR656ModeSelect(E_DRVVPOST_CCIR656_MODE eMode);
extern VOID vpostSetParalelSyncLCM_Interface(E_DRVVPOST_PARALLEL_SYNCLCM_INTERFACE eInterface);
extern VOID vpostSetFrameBuffer_DataType(E_DRVVPOST_FRAME_DATA_TYPE eType);
extern VOID vpostSetFrameBuffer_BaseAddress(UINT32 u32BufferAddress);
extern VOID vpostSetYUVEndianSelect(E_DRVVPOST_ENDIAN eEndian);
extern VOID vpostSetDataBusPin(E_DRVVPOST_DATABUS eDataBus);
extern VOID vpostSetSyncLCM_HTiming(S_DRVVPOST_SYNCLCM_HTIMING *psHTiming);
extern VOID vpostSetSyncLCM_VTiming(S_DRVVPOST_SYNCLCM_VTIMING *psVTiming);
extern VOID vpostSetSyncLCM_ImageWindow(S_DRVVPOST_SYNCLCM_WINDOW *psWindow);
extern VOID vpostSetSyncLCM_SignalPolarity(S_DRVVPOST_SYNCLCM_POLARITY *psPolarity);
extern VOID vpostSetInterlaceMode(BOOL bTvInter);
extern VOID vpostSetTVEnableConfig(UINT8 u8FBSIZE, UINT8 u8LCDSource, UINT8 u8TVSource, 
							BOOL bTvDAC, BOOL bTvInter, BOOL bTvSystem, BOOL bTvEncoderEnable);
extern VOID vpostSetTVSize(UINT8 u8FBSIZE);
extern VOID vpostSetLCM_ImageSource(E_DRVVPOST_IMAGE_SOURCE eSource);
extern VOID vpostMPULCDWriteAddr16Bit(unsigned short u16AddrIndex);
extern VOID vpostMPULCDWriteData16Bit(unsigned short  u16WriteData);
extern UINT16 vpostMPULCDReadData16Bit(void);
extern VOID vpostEnableInt(E_DRVVPOST_INT eInt);
extern VOID vpostDisableInt(E_DRVVPOST_INT eInt);
extern VOID vpostClearInt(E_DRVVPOST_INT eInt);
extern BOOL vpostIsIntEnabled(E_DRVVPOST_INT eInt);
extern int vpostInstallCallBack(
		E_DRVVPOST_INT eIntSource,
		PFN_DRVVPOST_INT_CALLBACK	pfnCallback,
		PFN_DRVVPOST_INT_CALLBACK 	*pfnOldCallback
	);
extern VOID vpostSetMPULCM_ImageWindow(S_DRVVPOST_MPULCM_WINDOW *psWindow);
extern VOID vpostSetMPULCM_TimingConfig(S_DRVVPOST_MPULCM_TIMING *psTiming);
extern VOID vpostSetMPULCM_BusModeSelect(E_DRVVPOST_MPULCM_DATABUS eBusMode);
extern void vpostSetOSD_Enable(void);
extern void vpostSetOSD_Disable(void);
extern void vpostSetOSD_Size(S_DRVVPOST_OSD_SIZE* psSize);
extern void vpostSetOSD_Pos(S_DRVVPOST_OSD_POS* psPos);
extern void vpostSetOSD_DataType(E_DRVVPOST_OSD_DATA_TYPE eType);
extern int vpostSetOSD_Transparent(E_DRVVPOST_OSD_TRANSPARENT_DATA_TYPE eType, UINT32 u32Pattern);
extern void vpostSetOSD_BaseAddress(UINT32 u32BaseAddress);
extern void vpostSetOSD_FunctionConfig(S_DRVVPOST_OSD_CTRL* psOSD);


