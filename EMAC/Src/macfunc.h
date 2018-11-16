/****************************************************************************
* Copyright (c) 2012-2015 Nuvoton Electronics Corp.
* All rights reserved.
*
* FILE NAME: mac.h
*
* DESCRIPTION: Header File for MAC.
*
****************************************************************************/
#ifndef W55FA92_MAC_FUNC_H
#define W55FA92_MAC_FUNC_H

#include "maclib.h"

// Utilities
extern void ReadErrReport(int num) ;
extern void ClearErrReport(int num) ;

// Ethernet and MAC hardware control functions
extern void LanInitialize(int num) ;
extern void MacInitialize(int num) ;
extern void TxFDInitialize(int num) ;
extern void RxFDInitialize(int num) ;
extern void ReadyMac(int num) ;
extern void MacTxGo(int num) ;
extern void MacRxOff(int num) ;
extern void SetMacAddr(int num) ;
extern void FillCamEntry(int num, int entry, UINT32 msw, UINT32 lsw);
extern void EnableCamEntry(int num, int entry);
extern void DisableCamEntry(int num, int entry);
extern void ResetPhyChip(int num,int mode) ;
extern void ResetPhyChip_ONE(int num) ;
extern void AutoDetectPhyAddr(void);
extern void MiiStationWrite(int num, UINT32 PhyInAddr, UINT32 PhyAddr, UINT32 PhyWrData) ;
extern UINT32  MiiStationRead(int num, UINT32 PhyInAddr, UINT32 PhyAddr) ;
extern void MAC0_Tx_isr(void) ;
extern void MAC0_Rx_isr(void) ;
extern void MAC1_Tx_isr(void) ;
extern void MAC1_Rx_isr(void) ;
extern int  SendPacket(int num, UINT8 *Data,int Size) ;
extern void SetMACport(int num, int port);

#define _DIAG
#ifdef _DIAG
// MAC diagnostic test functions
extern int  MacTest(void) ;
extern int  BISTModeTest(void) ;
extern int  RegisterTest(void) ;
extern int  MacFunctionTest(void) ;

// MAC function test
extern int  MacLoopBackTest(void) ;
extern int  CamCaptureTest(void) ;
extern int  TxRxMacFrame(void) ;
extern int  SpecialPacketTest(void) ;
extern int  MiscFunctionTest(void) ;

// Register test functions
extern int  RegDefaultTest(int num) ;
extern int  RegRWTest(int num) ;
extern int  SWRTest(int num) ;

// Loopback test
extern int  MacInternalLoopBack(int num) ;
extern int  PhyLoopBack(int num) ;
extern int  MacPollLoopBack(int num) ;

// CAM compare test functions
extern int  CAM_Entry_Test(int num,int entry);

// Special Packet Test
extern int  PauseFrameCheck(int num) ;
extern int  LongFrameCheck(int num) ;
extern int  ShortFrameCheck(int num) ;
extern int  CRCEFrameCheck(int num) ;
extern int  MissPacketCheck(int num) ;

// Misc Function Test
extern int  StationManageTest(int num);
extern int  StripeCRCTest(int num);
extern int  RxOverflowCheck(int num) ;
extern int  MemBusErrorCheck(int num) ;
extern int  DENDFOCheck(int num) ;

// Utilities
extern int  InterruptTxRxTest(int num) ;
extern int  InterruptTxRxTest_NAT(int port, int num, UINT8 *srcpkt, UINT8 *pattern) ;
extern int  PollingLoopBackTest(int num,int cnt) ;
extern int  TxRxOnePacket(int num, int size, int type) ;
extern int  Random_Pkt_Transmit(int num, int size, UINT8 *MACAddr) ;
extern int  CompareTxRxData(int num, UINT8 *pTxFrameData, UINT32 UserFrameBufferPtr) ;
extern int  CompareTxRxData_NAT(int num, UINT8 *pTxFrameData, UINT32 UserFrameBufferPtr, int FrameLength) ;
extern void GetRxFrameData(int num,UINT8 *pFrameData,UINT32 FrameLength,UINT32 RxStatus);
extern void GetRxFrameData_NAT(int num, UINT8 *pFrameData, UINT32 FrameLength);
extern void CaptureRxFrame(int num, int entry) ;
extern int  CompareRecevedAddr(int num, int Option);
extern void ReadErrReport(int num) ;
extern void ClearErrReport(int num) ;
extern int  bcomp(void *src, void *dst, int tsize) ;
extern UINT32  swap32(UINT32 data);
#endif /* _DIAG */

#endif /* _MAC_FUNC_ */
