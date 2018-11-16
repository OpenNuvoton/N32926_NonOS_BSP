/**************************************************************************************************
 *                                                                          
 * Copyright (c) 2012 - 2015 Nuvoton Electronics Corp. All rights reserved.      
 *                                                                         
 * FILENAME
 *     mac.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     Main functions of MAC Diagnostic Program. This program will receive any packets 
 *     from one Ethernet port, then send the packets through another port. The symbol 
 *     EXTERNAL_LOOPBACK_PORT define the port for sending packets. When running this program,
 *     please plug a loop-back connector at the Ethernet port (EXTERNAL_LOOPBACK_PORT) and connect
 *     the other port to a LAN or a test machine for receiving packets.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *
 * HISTORY
 *	   10/11/2012		 Port to W55FA92 by PX10 Chris
 *     12/27/2012		 Sample code for W55FA92 EMAClib	
 *
 * REMARK
 *     None
 *     
 *************************************************************************************************/
#include <stdarg.h>
#include <string.h>
#include "maclib.h"

#include "wblib.h"
#define UART_printf sysPrintf
//volatile uint32 cur_ticks = 0; //1 sec = 100 ticks
#define cur_ticks sysGetTicks(TIMER0)


#define LOOP_BACK_TEST

// fixed buffers, just for testing !
static UINT32 rxFDBaseAddr;     //descriptor buffer
static UINT32 txFDBaseAddr;
static UINT32 rxFBABaseAddr;     //data buffer
static UINT32 txFBABaseAddr;

                 
static volatile UINT32 myCAMCMR;

// For export report information
// Global variable structure for store status
extern volatile UINT32 gTxErrPacketCnt[2]; 
extern volatile UINT32 gRxErrPacketCnt[2]; 
extern volatile UINT32 gRxCRCErrOnDMAPacketCnt[2];
extern volatile S_MACTxStatus gsMacTxStatus[2];
extern volatile S_MACRxStatus gsMacRxStatus[2];

static volatile UINT32 PreTicks[2];
static volatile UINT32 PreTxBytes[2], PreRxBytes[2];

static UINT32 TxPacketBuf;
//static UINT8 srcMAcAddr[6];

static NVT_EMAC_T emacSetting;


void ShowTxRxStatusWithTime()
{
  volatile UINT32 t_hr, t_min, t_sec;
  volatile UINT32 TxRate[2], RxRate[2];
  UINT32 ticks;
  int num, i;
  
  ticks = cur_ticks; //get current tick count !  
  for (num=0; num<1; num++)
  {
       RxRate[num] = ((gsMacRxStatus[num].RxBytes - PreRxBytes[num]) * 100 * 8) / ((ticks - PreTicks[num]) * 1024);
       TxRate[num] = ((gsMacTxStatus[num].TxBytes - PreTxBytes[num]) * 100 * 8) / ((ticks - PreTicks[num]) * 1024);
       PreTxBytes[num] = gsMacTxStatus[num].TxBytes;
       PreRxBytes[num] = gsMacRxStatus[num].RxBytes;
       PreTicks[num]   = ticks;
  }
  
  t_sec = ticks / 100;
  t_min = t_sec / 60;
  t_sec %= 60;
  
  t_hr = t_min / 60;
  t_min %= 60;

// UART_printf("\n\n");
  UART_printf("                 RUNNING TIME - %2d : %2d : %2d\n", t_hr, t_min, t_sec);
  
  for (num = 0; num<1; num++)
  {
    UART_printf("MAC %d ------------------------------------------------------------------------\n", num);
    UART_printf("[1] Throughput| Tx:%6d Kbps    Rx:%6d Kbps\n", TxRate[num], RxRate[num]);
    UART_printf("[2] PKT Count | Tx:%d         Rx:%d\n", gsMacTxStatus[num].TXCP, gsMacRxStatus[num].RXGD);
    UART_printf("[3] ERR Pkts  | Tx:%d         Rx:%d         RxCRCOnDMA:%d\n", gTxErrPacketCnt[num], gRxErrPacketCnt[num], gRxCRCErrOnDMAPacketCnt[num]);
    UART_printf("[4] TX Status | DEF:%d, EDEF:%d, NCS:%d, ABT:%d, LC:%d", gsMacTxStatus[num].DEF, gsMacTxStatus[num].EXDEF, gsMacTxStatus[num].NCS, gsMacTxStatus[num].TXABT, gsMacTxStatus[num].LC);
//    UART_printf("              ");
    UART_printf(", HA:%d, PAU:%d, SQE:%d, BErr:%d, TDU:%d, EMP:%d\n", gsMacTxStatus[num].TXHA, gsMacTxStatus[num].PAU, gsMacTxStatus[num].SQE, gsMacTxStatus[num].TxBErr, gsMacTxStatus[num].TDU, gsMacTxStatus[num].EMP);
    UART_printf("[5] RX Status | RP:%d, MMP:%d, PTL:%d, CRCE:%d, CFR:%d, BErr:%d, RXOV:%d", gsMacRxStatus[num].RP, gsMacRxStatus[num].MMP, gsMacRxStatus[num].PTLE,
                                                                 gsMacRxStatus[num].CRCE, gsMacRxStatus[num].CFR, gsMacRxStatus[num].RxBErr, gsMacRxStatus[num].RXOV);
//    UART_printf("              "); 
	UART_printf(", ALI:%d\n", gsMacRxStatus[num].ALIE);
  }

  for (i=0; i<13; i++)
  	UART_printf("\n");

  UART_printf("%c[21A", 0x1B);
  //UART_printf("\n\n");
}
 

// Read Error Status and Time
void ReadErrReport(int num)
{
 UART_printf("< Error Report >\n") ;
 UART_printf("MAC Tx Err Count (Good:%d)\n",(int)gsMacTxStatus[num].TXCP) ;
 UART_printf("TXABT: %d, DEF: %d, PAU: %d,\n", (int)gsMacTxStatus[num].TXABT,
             (int)gsMacTxStatus[num].DEF, (int)gsMacTxStatus[num].PAU);
 UART_printf("EXDEF: %d, NCS: %d, SQE: %d,\n", (int)gsMacTxStatus[num].EXDEF,
             (int)gsMacTxStatus[num].NCS, (int)gsMacTxStatus[num].SQE);
 UART_printf("LC: %d, TXHA: %d\n",
               (int)gsMacTxStatus[num].LC,(int)gsMacTxStatus[num].TXHA);

 UART_printf("MAC Rx Err Count (Good:%d)\n",(int)gsMacRxStatus[num].RXGD) ;
 UART_printf("ALIE: %d, CRCE: %d,\n",
            (int)gsMacRxStatus[num].ALIE, (int)gsMacRxStatus[num].CRCE);
 UART_printf("PTLE: %d, RP: %d\n",
            (int)gsMacRxStatus[num].PTLE, (int)gsMacRxStatus[num].RP);

 UART_printf("Missed Error Count : %d\n",MPCNT_0) ;

}


// Clear Error Report Area
void ClearErrReport(int num)
{
 gsMacTxStatus[num].DEF=gsMacTxStatus[num].TXCP=gsMacTxStatus[num].EXDEF=0;
 gsMacTxStatus[num].NCS=gsMacTxStatus[num].TXABT=gsMacTxStatus[num].LC=0;
 gsMacTxStatus[num].TXHA=gsMacTxStatus[num].PAU=gsMacTxStatus[num].SQE=0;
 gsMacTxStatus[num].TxBErr = 0; //CMN [2002/11/01]
 gsMacTxStatus[num].TxBytes = 0;
 gsMacTxStatus[num].TDU = 0;
 gsMacTxStatus[num].EMP = 0;

 gsMacRxStatus[num].RP=gsMacRxStatus[num].ALIE=gsMacRxStatus[num].RXGD=0;
 gsMacRxStatus[num].PTLE=gsMacRxStatus[num].CRCE=gsMacRxStatus[num].CFR=0; 
 gsMacRxStatus[num].RxBErr = 0; //CMN [2002/11/01]
 gsMacRxStatus[num].RxBytes = 0;
 gsMacRxStatus[num].RXOV = 0;
 
#ifdef USE_TIME
 PreTicks[num] = 0;
 PreTxBytes[num] = PreRxBytes[num] = 0;
#endif  
}


int InitSendPacket()
{
  int i;
  volatile UINT8 *ptr;

  ptr = (UINT8 *)TxPacketBuf;
  
#if 1
  //dst MAC address, the Test Machine H/W MAC Address
  #if 0
  *ptr++ = 0x00;
  *ptr++ = 0x50;
  *ptr++ = 0xFC;
  *ptr++ = 0x78;
  *ptr++ = 0x48;
  *ptr++ = 0x06;
  #else
  *ptr++ = 0x00;
  *ptr++ = 0x01;
  *ptr++ = 0x6C;
  *ptr++ = 0x0C;
  *ptr++ = 0xAB;
  *ptr++ = 0xD2;
  #endif
#else
  //Dst MAC address
  *ptr++ = 0x00;
  *ptr++ = 0x50;
  *ptr++ = 0xBA;
  *ptr++ = 0x33;
  *ptr++ = 0xBE;
  *ptr++ = 0x44;
#endif
  
  //Src MAC address
  *ptr++ = 0x00;
  *ptr++ = 0x50;
  *ptr++ = 0xBA;
  *ptr++ = 0x33;
  *ptr++ = 0xBE;
  *ptr++ = 0x44;
  
  //IP protocol
  *ptr++ = 0x08;
  *ptr++ = 0x00;
  
  //dummy
  *ptr++ = 0x00;
  *ptr++ = 0x5A;

  // packSeq 4 bytes
  ptr+=4;
	
  //data
  for (i=0; i<100; i++)
     //*ptr++ = i & 0xFF;
     *ptr++ = 0x6A;

  return 0;

}


void _InitVar()
{

#if 0
#ifdef CACHE_ON
	// fixed buffers, just for testing !
	rxFDBaseAddr=0x80400000;     //descriptor buffer
	txFDBaseAddr=0x80400200;	 
	rxFBABaseAddr=0x80400300;     //data buffer
	txFBABaseAddr=0x8040c300;
	TxPacketBuf=0x80500000;
#else
// fixed buffers, just for testing !
	rxFDBaseAddr=0x400000;     //descriptor buffer
	txFDBaseAddr=0x400200;
	rxFBABaseAddr=0x400300;     //data buffer
	txFBABaseAddr=0x40c300;
	TxPacketBuf=0x500000;
#endif
#endif

  static __align(4) UINT8 rxFDBuff[sizeof(S_FrameDescriptor)*MaxRxFrameDescriptors]; //[0x200];	// 16x32
  static __align(4) UINT8 txFDBuff[sizeof(S_FrameDescriptor)*MaxTxFrameDescriptors]; //[0x100];	// 16x16
  static __align(4) UINT8 rxFBBuff[sizeof(S_MACFrame)*MaxRxFrameDescriptors]; //[0xC000]; //0x600 x 32 
  static __align(4) UINT8 txFBBuff[sizeof(S_MACFrame)*MaxTxFrameDescriptors]; //[0x6000]; //0x600 x 16
  static __align(4) UINT8 txPaBuff[sizeof(S_MACFrame)]; //[0x600];
  
  rxFDBaseAddr = (UINT32)rxFDBuff;
  txFDBaseAddr = (UINT32)txFDBuff;
  rxFBABaseAddr = (UINT32)rxFBBuff;
  txFBABaseAddr = (UINT32)txFBBuff;
  TxPacketBuf = (UINT32)txPaBuff;

#ifdef CACHE_ON
	rxFDBaseAddr |= 0x80000000;
	txFDBaseAddr |= 0x80000000;
	rxFBABaseAddr |= 0x80000000;
	txFBABaseAddr |= 0x80000000;
	TxPacketBuf  |= 0x80000000;
#endif

	// Global variables  used for MAC driver
	                 
	//myCAMCMR = CAM_ECMP; //|CAM_AUP;
	myCAMCMR = CAM_ECMP | CAM_AUP;
	//Accept ANY kind of packet (Unicast, Broadcasr and Multicase) !!!
	//myCAMCMR = CAM_ECMP | CAM_AUP | CAM_AMP | CAM_ABP;

	emacSetting.srcMAcAddr[0] = 0x00;
	emacSetting.srcMAcAddr[1] = 0x50;
	emacSetting.srcMAcAddr[2] = 0xba;
	emacSetting.srcMAcAddr[3] = 0x33;
	emacSetting.srcMAcAddr[4] = 0xbe;
	emacSetting.srcMAcAddr[5] = 0x44;	
	
	emacSetting.speedMode = MODE_DR100_FULL; //MODE_DR10_FULL;
	emacSetting.recvPacketType = myCAMCMR;
	emacSetting.rxFDBaseAddr = rxFDBaseAddr;
	emacSetting.txFDBaseAddr = txFDBaseAddr;
	emacSetting.rxFBABaseAddr = rxFBABaseAddr;
	emacSetting.txFBABaseAddr = txFBABaseAddr;
	emacSetting.portNo = 0;
}


void _recvRxFrame(UINT8 *pRxFrameData, UINT32 frameLength)
{

	UINT8 *pData = (pRxFrameData + frameLength -1);

#if 0 // packSeq is only for HW verification with specific lib	
	static UINT32 packSeq=0;
	UINT32 curSeq;
	
	curSeq =  *((UINT32 *)(pRxFrameData + 16));
//	UART_printf(" Got packet seq=[0x%x]\n", curSeq);
#ifdef LOOP_BACK_TEST
	if( *((UINT32 *)(pRxFrameData + 16)) != packSeq++ )
	{
		UART_printf(" ===ERR: Got wrong packet sequence num=[0x%x], should be [0x%x]\n", curSeq, packSeq-1);
	}
#endif	
#endif
//	UART_printf("Recv RX frameLength=%d\n",frameLength);
//	UART_printf("Recv RX last bytes [%x] [%x] [%x] [%x] [%x]\n", 
//					*(pData-4),*(pData-3),*(pData-2),*(pData-1),*pData);  
}

// Main function for MAC Block test
int MacTest(void)
{	
// volatile unsigned int loop=1;  
 volatile UINT32 btime;
// volatile UINT32 total_time;
 unsigned int volatile ii;

 
 {
	UINT32 u32ExtFreq;	
	u32ExtFreq = sysGetExternalClock();
	sysSetTimerReferenceClock(TIMER0, u32ExtFreq); //External Crystal	
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);			/* 100 ticks/per sec ==> 1tick/10ms */

 }
 
 ClearErrReport(0);
 _InitVar();

 //Initialize tx packet
 InitSendPacket();
 
 EMAC_Init(&emacSetting);

 // WakeOnLan (WOL) test ==========================
#ifndef LOOP_BACK_TEST
 UART_printf("\n Wake on Lan test, enter power down & wait for magic packet ... \n");
 EMAC_EnableWakeOnLan();
// outpw(GCR_BA+0x20, inpw(GCR_BA+0x20) | 0x100);	// power down
 outpw(GCR_BA+0x20, inpw(GCR_BA+0x20) | 0x10);	// power down
  btime = cur_ticks;
 while(1)
 {
   if ((cur_ticks - btime) >= 200)
   {
   		btime = cur_ticks;
 		UART_printf(".");
 		if( inpw(GCR_BA+0x34) & 0x4000 )    // To ensure the wake up by EMAC
 			break;
   }		
 }
//  outpw(GCR_BA+0x20, inpw(GCR_BA+0x20) & ~0x100);	// power down
  outpw(GCR_BA+0x20, inpw(GCR_BA+0x20) & ~0x10);	// power down
 UART_printf("\n Got magic packet ! \n");
 EMAC_Init(&emacSetting);
#endif 
 //================================================

 btime = cur_ticks;

 EMAC_SetRxCallBack((PVOID)_recvRxFrame);
 
 while(1) 
 {
 	if(ii++ == 0)
   		EMAC_SendPacket((UINT8 *)TxPacketBuf, 120);
    ii &= 0x1fff;    
   if ((cur_ticks - btime) >= 200)
   {
//       total_time = cur_ticks - btime;
       btime = cur_ticks;
       ShowTxRxStatusWithTime();       
   }
   
 }
	
 EMAC_Exit();
 
 return 1;
}


int main()
{
//  unsigned int oldvect, temp;

	WB_UART_T uart;
	UINT32 u32ExtFreq;
	u32ExtFreq = sysGetExternalClock();
	sysUartPort(1);
	uart.uiFreq = u32ExtFreq;	//use APB clock
    	uart.uiBaudrate = 115200;
    	uart.uiDataBits = WB_DATA_BITS_8;
    	uart.uiStopBits = WB_STOP_BITS_1;
    	uart.uiParity = WB_PARITY_NONE;
    	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    	uart.uart_no = WB_UART_1;
    	sysInitializeUART(&uart);
    sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	

  
  UART_printf("\n\nW55FA92 DIAG Program 1.0 - MAC\n\n");


  
#ifdef CACHE_ON
   sysInvalidCache();
   sysEnableCache(0); //0 for write-back, 1 for write through
   UART_printf("\n\n Cache Mode : Write-Back\n\n");
   
   sysFlushCache(8); //8 for I_D_CACHE
   //sysSetCachePages(0x400000, 0x100000, -1); // -1 for CACHE_DISABLE
#endif  //CACHE_ON

  
  MacTest();
  return 0;
}

