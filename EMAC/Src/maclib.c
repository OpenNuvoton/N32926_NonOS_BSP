/****************************************************************************
* Copyright (c) 20012-2015 Nuvoton Electronics Corp.
* All rights reserved.
*
* FILE NAME: maclib.c
*
* DESCRIPTION: MAC Driver Routines.
*
****************************************************************************/
#include <stdarg.h>
#include <string.h>
#include "wblib.h"
#include "maclib.h"
#include "macfunc.h"


#define UART_printf sysPrintf

// Frame buffer definition for restore received buffer
#define RxFrameDataPreamble 0xCCCCCCCC


static UINT32 m_RxFDBaseAddr0;  //descriptor buffer
static UINT32 m_TxFDBaseAddr0;  
static UINT32 m_RxFBABaseAddr0; //data buffer
static UINT32 m_TxFBABaseAddr0; 


volatile UINT32 gCTxFDPtr[2], gWTxFDPtr[2], gCRxFDPtr[2];


#define AUTO_NEGOTH	1
//#define PHY_DM9161AE 1


// Pause Frame Multicast Address
UINT32 PauseMultiAddr0=0x0180c200;
UINT32 PauseMultiAddr1=0x00010000;
 
// Global variables  used for MAC driver
//sattic volatile UINT32 m_MCMDR = MCMDR_RXON | MCMDR_SPCRC | MCMDR_EnMDC | MCMDR_FDUP | MCMDR_EnRMII; // | MCMDR_AEP;
//static volatile UINT32 m_MCMDR = MCMDR_FDUP | MCMDR_SPCRC | MCMDR_RXON | MCMDR_EnMDC | MCMDR_AEP | MCMDR_EnRMII;
static volatile UINT32 m_MCMDR = MCMDR_RXON | MCMDR_EnMDC | MCMDR_FDUP | MCMDR_SPCRC | MCMDR_EnRMII | MCMDR_AEP | MCMDR_ARP | MCMDR_ALP;
                      
//static volatile UINT32 m_MIEN = EnTXINTR | EnTXCP | EnRXINTR | EnRXGD | 
//                     EnTxBErr | EnRxBErr | //EnTDU | EnRDU | 
//                     EnCRCE | EnRP | EnPTLE | EnRXOV | EnCFR |
//                     EnLC | EnTXABT | EnNCS | EnEXDEF | EnTXEMP;
static volatile UINT32 m_MIEN = EnTXINTR | EnRXINTR | EnRXGD | EnTXCP |
	                     EnTxBErr | EnRxBErr | EnRDU;// | EnTDU; //| EnALIE; //|EnCRCE ;                       
                     
static volatile UINT32 m_CAMCMR = CAM_ECMP | CAM_AUP;

volatile UINT32 gCTxFDPtr[2], gWTxFDPtr[2], gCRxFDPtr[2];
volatile UINT32 gCam0M_0 = 0 , gCam0L_0 = 0;
volatile UINT32 gCam0M_1 = 0 , gCam0L_1 = 0;
volatile UINT8  MyMacSrcAddr[2][6] ;
volatile int gErrorPacketCnt[2] ;
volatile UINT32 gTxErrPacketCnt[2]; //CMN
volatile UINT32 gRxErrPacketCnt[2]; //CMN
volatile UINT32 gRxCRCErrOnDMAPacketCnt[2];
volatile int MacRxDoneFlagForLoopBackCheck[2] ;
volatile int MacTxDoneFlagForLoopBackCheck[2] ;

// Global variable structure for store status
volatile S_MACTxStatus gsMacTxStatus[2];
volatile S_MACRxStatus gsMacRxStatus[2];
volatile unsigned int PHYAD;

volatile int DENIflag=0, DFOIflag=0, RXOVflag=0;

#ifdef _W55FA92_
volatile int BOflag = 0;  //Byte offset enable flag [0519]
#endif


volatile UINT32 PktSeq = 0;
volatile UINT32 TxPktSeq = 0;
volatile UINT32 RxPktSeq = 0;
volatile UINT32 TxPktSeqWanted = 0;
volatile UINT32 RxPktSeqWanted = 0;
volatile UINT32 TxPktSeqErr = 0;
volatile UINT32 RxPktSeqErr = 0;
volatile UINT32 DoChk = 0;

int m_mode = MODE_DR100_FULL;

void AutoDetectPhyAddr()
{
    int i;
    int flag, num = 0;
    unsigned int temp;
    
    flag = 0;
    PHYAD = 0;
	for (i=0; i<32; i++)
	{
		temp = MiiStationRead(num, 0, (i << 8));
		if ((temp & 0xFFFF) != 0xFFFF)
		{
			PHYAD = i << 8;			
			flag = 1;			
			break;
		}
		//UART_printf("PHY ADDR %d = %x\n", i, MiiStationRead(num, 0, (i << 8)));	
	}	
	if (flag == 1)
		UART_printf("AutoDetectPhyAddr() : PHY ADDR = %d\n", i);
	else
		UART_printf("AutoDetectPhyAddr() : Don't find PHY Addr!\n");

} /* end AutoDetectPhyAddr */
	

void SetPhyRmiiTiming(int num)
{
#ifdef PHY_RTL8201F
	UART_printf("Set RTL8201 PHY RMII Timimg\n");
 	MiiStationWrite(num, 31, PHYAD, 7);//MIKE Chiang (For RTL8201F)
 	MiiStationWrite(num, 17, PHYAD, 0x12);//MIKE Chiang (For RTL8201F)
 	if (MiiStationRead(num, 17, PHYAD)!=0x12) UART_printf("RTL8201 Setting Fail...\n");//MIKE Chiang (For RTL8201F)
 	MiiStationWrite(num, 31, PHYAD, 7);//MIKE Chiang (For RTL8201F)
 	MiiStationWrite(num, 19, PHYAD, 0x38);//MIKE Chiang (For RTL8201F)
 	if (MiiStationRead(num, 19, PHYAD)!=0x38) UART_printf("RTL8201 Setting Fail...\n");//MIKE Chiang (For RTL8201F)
 	MiiStationWrite(num, 31, PHYAD, 7);//MIKE Chiang (For RTL8201F)
 	MiiStationWrite(num, 16, PHYAD, 0x79A);//MIKE Chiang (For RTL8201F)
 	if (MiiStationRead(num, 16, PHYAD)!=0x79A) UART_printf("RTL8201 Setting Fail...\n");//MIKE Chiang (For RTL8201F)
#endif
}
	

// Reset PHY, Auto-Negotiation Enable
void ResetPhyChip(int num,int mode)
{
 UINT32 volatile RdValue, loop;
 UINT32 volatile regANA, regANLPA;
 int    speed=10;
// int i;


#if 1
 /* [2005/07/28] by cmn */
 RdValue = MiiStationRead(num, 16, PHYAD) ;
 if (!(RdValue & 0x0100))
 {
 	UART_printf("WARNING, RMII is not enabled, set it by software !\n");
 	RdValue |= 0x0100;
 	MiiStationWrite(num, 16, PHYAD, RdValue); 	 
 }
#endif

#ifdef AUTO_NEGOTH  //cmn [0521]
 UART_printf("Reset PHY...\n");
 
 MiiStationWrite(num, PHY_CNTL_REG, PHYAD, RESET_PHY); 
 while (1)
 {
 	RdValue = MiiStationRead(num, PHY_CNTL_REG, PHYAD) ;
    if ((RdValue&RESET_PHY)==0)
      break;
 }
 SetPhyRmiiTiming(num);
 //UART_printf("PHY 1, CTRL REG   = %x\n",MiiStationRead(num, PHY_CNTL_REG, PHYAD));
 //UART_printf("PHY 1, STATUS REG = %x\n",MiiStationRead(num, PHY_STATUS_REG, PHYAD)); 

 switch(mode)
 {
  case MODE_DR100_FULL: 
        MiiStationWrite(num,PHY_ANA_REG,PHYAD,DR100_TX_FULL|IEEE_802_3_CSMA_CD);
        break;
  case MODE_DR100_HALF: 
        MiiStationWrite(num,PHY_ANA_REG,PHYAD,DR100_TX_HALF|IEEE_802_3_CSMA_CD);
        break;
  case MODE_DR10_FULL: 
        MiiStationWrite(num,PHY_ANA_REG,PHYAD,DR10_TX_FULL|IEEE_802_3_CSMA_CD);
        break;
  case MODE_DR10_HALF: 
        MiiStationWrite(num,PHY_ANA_REG,PHYAD,DR10_TX_HALF|IEEE_802_3_CSMA_CD);
        break;
  default: 
        break;
 }	   
                     
 RdValue = MiiStationRead(num, PHY_CNTL_REG, PHYAD) ;
 RdValue |= RESTART_AN;
 MiiStationWrite(num, PHY_CNTL_REG, PHYAD, RdValue); 
 //MiiStationWrite(num, PHY_CNTL_REG, PHYAD, ENABLE_AN | RESTART_AN); 

 UART_printf("ResetPhyChip(): wait auto-negotiation bit cleared....\n");
 
 //UART_printf("CTRL REG = %x\n",MiiStationRead(num, PHY_CNTL_REG, PHYAD));
 
 while (1) // wait for auto-negotiation complete
   {
    RdValue = MiiStationRead(num, PHY_STATUS_REG, PHYAD) ;
    if ((RdValue&AN_COMPLETE)!=0)
      break;
   }
   
  /* wait for link valid [2006/06/06] */
 while ( !(MiiStationRead(num, PHY_STATUS_REG, PHYAD) & 0x04) ) ; 
 
#if  0
 //MiiStationWrite(num, 16, PHYAD, 0x0114); 
 //MiiStationWrite(num, 18, PHYAD, 0x2000); 
 UART_printf("REG 16 = %x\n",MiiStationRead(num, 16, PHYAD));
 UART_printf("REG 17 = %x\n",MiiStationRead(num, 17, PHYAD));
 UART_printf("REG 18 = %x\n",MiiStationRead(num, 18, PHYAD));
 //UART_printf("REG 20 = %x\n",MiiStationRead(num, 20, PHYAD));
#endif 

 regANA   = MiiStationRead(num, PHY_ANA_REG, PHYAD);
 regANLPA = MiiStationRead(num, PHY_ANLPA_REG, PHYAD);

 if ((regANA & 0x100) && (regANLPA & 0x100)) //100Mb, Full duplex
 {
	  UART_printf("100MB : Full Duplex\n");
	  speed =100;
      MCMDR_0 = MCMDR_0 | MCMDR_OPMOD | MCMDR_FDUP;           
 }
 else if ((regANA & 0x80) && (regANLPA & 0x80)) //100Mb, Half duplex
 {
	  UART_printf("100MB : Half Duplex\n");
	  speed =100;
      MCMDR_0 |= MCMDR_OPMOD;
      MCMDR_0 &= ~MCMDR_FDUP;

 }
 else if ((regANA & 0x40) && (regANLPA & 0x40)) //10Mb, Full duplex
 { 
	UART_printf("10MB : Full Duplex\n");
	speed =10;
      MCMDR_0 &= ~MCMDR_OPMOD;
      MCMDR_0 |= MCMDR_FDUP;

 }
 else //10Mb, half duplex
 {
	UART_printf("10MB : Half Duplex\n");
	speed =10;
      MCMDR_0 &= ~MCMDR_OPMOD;
      MCMDR_0 &= ~MCMDR_FDUP; 

 }

 //UART_printf("Wait a key..\n");
 //UART_getchar();
 
#else //Force Mode
  RdValue = MiiStationRead(num, PHY_CNTL_REG, PHYAD) ; 
  RdValue &= 0xC0FF;
 switch(mode)
 {
  case MODE_DR100_FULL: 
        RdValue = (RdValue | DR_100MB | PHY_FULLDUPLEX);
        break;
  case MODE_DR100_HALF: 
        RdValue = (RdValue | DR_100MB);
        break;
  case MODE_DR10_FULL: 
        RdValue = (RdValue | PHY_FULLDUPLEX);
        break;
  case MODE_DR10_HALF:               
  default: 
        break;
 }	   
 
  /* [2005/07/31] by cmn */
 //RdValue |= ENABLE_LOOPBACK; //for 100M test
 
 MiiStationWrite(num, PHY_CNTL_REG, PHYAD, RdValue);
 SetPhyRmiiTiming(num);  
 /* wait for link valid [2006/06/06] */
 while ( !(MiiStationRead(num, PHY_STATUS_REG, PHYAD) & 0x04) ) ; 
    
  
 //UART_printf("W55FA92 MAC%d: ",num);
 RdValue = MiiStationRead(num, PHY_CNTL_REG, PHYAD) ;
 UART_printf("CTRL_REG = %x\n", RdValue);
//// UART_printf("REG 20 = %x\n",MiiStationRead(num, 20, PHYAD));

 if ((RdValue&DR_100MB)!=0) // 100MB
   {
      UART_printf("100MB - ");
      speed =100;
	MCMDR_0 |= MCMDR_OPMOD;     
      
   }
  else 
   {
    UART_printf("10MB - ");
    speed =10;
      MCMDR_0 &= ~MCMDR_OPMOD;
   }
 if ((RdValue&PHY_FULLDUPLEX)!=0) // Full Duplex
   {
      UART_printf("Full Duplex\n");

      MCMDR_0 |= MCMDR_FDUP;
   }
  else 
   { 
      UART_printf("Half Duplex\n");

      MCMDR_0 &= ~MCMDR_FDUP;
   } 
#endif


 if (speed==100) // 100MB
   {
//      UART_printf("100MB - ");
#ifdef _W55FA92_   
 	*((unsigned volatile int *) (CLK_BA+0x40)) &= ~(0x13000000); // clean divider of 10Mbps
 	*((unsigned volatile int *) (CLK_BA+0x40)) |= 0x01000000; // CLK_DIV7: div PHY 50M as ref-clk for TX/RX 100Mbps 
#endif      
   }
  else 
   {
//    UART_printf("10MB - ");
#ifdef _W55FA92_   
 	*((unsigned volatile int *) (CLK_BA+0x40)) |= 0x13000000; // CLK_DIV7: div PHY 50M as ref-clk for TX/RX 10Mbps
#endif      
   }


#ifdef PHY_DM9161AE
   if ( !(MCMDR_0 & MCMDR_OPMOD) )
   			//Not a good method that this loop counter is just a guess, it may need to be modified!!!
#ifdef CACHE_ON		  
		   for (loop=0; loop<0x1000000; loop++) ;
#else
		   for (loop=0; loop<0x100000; loop++) ;
#endif		   
#endif   
     
}


// MII Interface Station Management Register Write
void MiiStationWrite(int num, UINT32 PhyInAddr, UINT32 PhyAddr, UINT32 PhyWrData)
{

    MIID_0 = PhyWrData ;
    MIIDA_0 = PhyInAddr | PhyAddr | PHYBUSY | PHYWR | MDCCR;
    while( (MIIDA_0 & PHYBUSY) )  ;
    /*
    UART_printf("MIID_0=%x\n",MIID_0);
    UART_printf("MIID_0=%x\n",MIID_0);
    UART_printf("MIIDA_0=%x\n",MIIDA_0);
    UART_printf("MIIDA_0=%x\n",MIIDA_0);
    UART_printf("MIID_0=%x\n",MIID_0);
    UART_printf("MIID_0=%x\n",MIID_0);
    */
    MIID_0 = 0 ;


}


// MII Interface Station Management Register Read
UINT32 MiiStationRead(int num, UINT32 PhyInAddr, UINT32 PhyAddr)
{
 UINT32 PhyRdData ;
 

    MIIDA_0 = PhyInAddr | PhyAddr | PHYBUSY | MDCCR;
    while( (MIIDA_0 & PHYBUSY) )  ;
    PhyRdData = MIID_0 ;  
    MIID_0 = 0 ;      

 return PhyRdData ;
}



// Set MAC Address to CAM
void SetMacAddr(int num)
{
 int i;


    /* Copy MAC Address to global variable */
    for (i=0;i<(int)MAC_ADDR_SIZE-2;i++)
       gCam0M_0 = (gCam0M_0 << 8) | MyMacSrcAddr[0][i] ;

    for (i=(int)(MAC_ADDR_SIZE-2);i<(int)MAC_ADDR_SIZE;i++)
       gCam0L_0 = (gCam0L_0 << 8) | MyMacSrcAddr[0][i] ;
    gCam0L_0 = (gCam0L_0 << 16) ;

    FillCamEntry(0, 0, gCam0M_0, gCam0L_0);

}


void EnableCamEntry(int num, int entry)
{

   CAMEN_0 |= 0x00000001<<entry ;

}


void DisableCamEntry(int num, int entry)
{

   CAMEN_0 &= ~(0x00000001<<entry) ;

}


void FillCamEntry(int num, int entry, UINT32 msw, UINT32 lsw)
{

    CAMxM_Reg_0(entry) = msw;
    CAMxL_Reg_0(entry) = lsw;

 EnableCamEntry(num,entry);
}



// Initialize Tx frame descriptor area-buffers.
void TxFDInitialize(int num)
{
 S_FrameDescriptor *pFrameDescriptor;
 S_FrameDescriptor *pStartFrameDescriptor;
 S_FrameDescriptor *pLastFrameDescriptor = NULL;
 UINT32 FrameDataAddr;
 UINT32 i;


    // Get Frame descriptor's base address.
    TXDLSA_0 = (UINT32)m_TxFDBaseAddr0;
    gWTxFDPtr[0] = gCTxFDPtr[0] = TXDLSA_0;

    // Get Transmit buffer base address.
    FrameDataAddr = (UINT32)m_TxFBABaseAddr0;

    // Generate linked list.
    pFrameDescriptor = (S_FrameDescriptor *) gCTxFDPtr[0];
    pStartFrameDescriptor = pFrameDescriptor;

    for(i=0; i < MaxTxFrameDescriptors; i++)
    {
     if (pLastFrameDescriptor == NULL)
       pLastFrameDescriptor = pFrameDescriptor;
     else
       pLastFrameDescriptor->NextFrameDescriptor = (UINT32)pFrameDescriptor;

     //pFrameDescriptor->Status1 = (PaddingMode | CRCMode);
     pFrameDescriptor->Status1 = (PaddingMode | CRCMode | MACTxIntEn);

#ifdef _W55FA92_
     if (BOflag)
         pFrameDescriptor->FrameDataPtr = (UINT32)(FrameDataAddr | (i % 4));
     else
#endif 
     pFrameDescriptor->FrameDataPtr = (UINT32)FrameDataAddr;
          
     pFrameDescriptor->Status2 = (UINT32)0x0;
     pFrameDescriptor->NextFrameDescriptor = NULL;

     pLastFrameDescriptor = pFrameDescriptor;
     pFrameDescriptor++;
     FrameDataAddr += sizeof(S_MACFrame);
    }

    // Make Frame descriptor to ring buffer type.
    pFrameDescriptor--;
    pFrameDescriptor->NextFrameDescriptor = (UINT32)pStartFrameDescriptor;

}


// Initialize Rx frame descriptor area-buffers.
void RxFDInitialize(int num)
{
 S_FrameDescriptor *pFrameDescriptor;
 S_FrameDescriptor *pStartFrameDescriptor;
 S_FrameDescriptor *pLastFrameDescriptor = NULL;
 UINT32 FrameDataAddr;
 UINT32 i;


    // Get Frame descriptor's base address.
    RXDLSA_0 = (UINT32)m_RxFDBaseAddr0;
    gCRxFDPtr[0] = RXDLSA_0;

    // Get Transmit buffer base address.
    FrameDataAddr = (UINT32)m_RxFBABaseAddr0;

    // Generate linked list.
    pFrameDescriptor = (S_FrameDescriptor *) gCRxFDPtr[0];
    pStartFrameDescriptor = pFrameDescriptor;

    for(i=0; i < MaxRxFrameDescriptors; i++)
    {
     if (pLastFrameDescriptor == NULL)
       pLastFrameDescriptor = pFrameDescriptor;
     else
       pLastFrameDescriptor->NextFrameDescriptor = (UINT32)pFrameDescriptor;

     pFrameDescriptor->Status1 = RXfOwnership_DMA;

#ifdef _W55FA92_
     if (BOflag)
         pFrameDescriptor->FrameDataPtr = (UINT32)(FrameDataAddr | (3 - (i % 4)));
     else
#endif      
     pFrameDescriptor->FrameDataPtr = (UINT32)FrameDataAddr;
     
     pFrameDescriptor->Status2 = (UINT32)0x0;
     pFrameDescriptor->NextFrameDescriptor = NULL;

     pLastFrameDescriptor = pFrameDescriptor;
     pFrameDescriptor++;
     FrameDataAddr += sizeof(S_MACFrame);
    }

    // Make Frame descriptor to ring buffer type.
    pFrameDescriptor--;
    pFrameDescriptor->NextFrameDescriptor = (UINT32)pStartFrameDescriptor;

}



// set Registers related with MAC.
void ReadyMac(int num)
{

    MIEN_0 = m_MIEN ;
    MCMDR_0 = m_MCMDR ;

}


// MAC Transfer Start for interactive mode
void MacTxGo(int num)
{

#if 0 
 UART_printf("MacTxGo() : PHY CtrlReg = %x\n", MiiStationRead(num, PHY_CNTL_REG, PHYAD));
#endif

 // Enable MAC Transfer

#ifdef INTERNAL_LOOPBACK  //[2005/07/27]
		MCMDR_0 |= MCMDR_LBK;
#endif  

    if (!(MCMDR_0&MCMDR_TXON))
      MCMDR_0 |= MCMDR_TXON ;
      
    TSDR_0 = 0;  


}


// Mac Rx Off and disable all interrupts
void MacRxOff(int num)
{

   MCMDR_0 &= ~MCMDR_RXON ;

// sysDisableInterrupt(EMCRXINT0) ;
}


// LAN Initialize Setting
void LanInitialize(int num)
{

 gErrorPacketCnt[num] = 0;
 gTxErrPacketCnt[num] = gRxErrPacketCnt[num] = 0; //CMN
 gRxCRCErrOnDMAPacketCnt[num] = 0;
 MacRxDoneFlagForLoopBackCheck[num] = 0;
 MacTxDoneFlagForLoopBackCheck[num] = 0;
#if 0
 MyMacSrcAddr[0][0] = 0x00 ;
 MyMacSrcAddr[0][1] = 0x50 ;
 MyMacSrcAddr[0][2] = 0xba ;
 MyMacSrcAddr[0][3] = 0x33 ;
 MyMacSrcAddr[0][4] = 0xbe ;
 MyMacSrcAddr[0][5] = 0x44 ;

 MyMacSrcAddr[1][0] = 0x00 ;
 MyMacSrcAddr[1][1] = 0x50 ;
 MyMacSrcAddr[1][2] = 0xba ;
 MyMacSrcAddr[1][3] = 0x33 ;
 MyMacSrcAddr[1][4] = 0xbe ;
 MyMacSrcAddr[1][5] = 0x55 ;
#endif
     
 // Initialize MAC controller
 MacInitialize(num) ;
 
 AutoDetectPhyAddr();
 ResetPhyChip(num,m_mode) ;   // MODE_DR10_HALF


 // Set MAC address to CAM
 SetMacAddr(num) ;
}





// Initialize MAC Controller
void MacInitialize(int num)
{
 //UART_printf("MacInitialize()\n");

    // MAC interrupt vector setup.
	sysInstallISR(IRQ_LEVEL_1, (INT_SOURCE_E)EMCTXINT0, (PVOID)MAC0_Tx_isr);
	sysInstallISR(IRQ_LEVEL_1, (INT_SOURCE_E)EMCRXINT0, (PVOID)MAC0_Rx_isr);

    // Set the Tx and Rx Frame Descriptor
    TxFDInitialize(num) ;
    RxFDInitialize(num) ;

    // Set the CAM Control register and the MAC address value
    FillCamEntry(0, 0, gCam0M_0, gCam0L_0);
    CAMCMR_0 = m_CAMCMR ;

    // Enable MAC Tx and Rx interrupt.
	sysEnableInterrupt((INT_SOURCE_E)EMCTXINT0);
	sysEnableInterrupt((INT_SOURCE_E)EMCRXINT0);

    // Configure the MAC control registers.
    ReadyMac(num) ;

}

typedef void (*sys_pvFunPtr)(UINT8 *pRxFrameData, UINT32 FrameLength);   /* function pointer */
sys_pvFunPtr m_pvRxCallBack=0;
UINT32 *m_RxFrameBasePtr;

// Move MAC Received Frame Data
void GetRxFrameData(int num, UINT8 *pFrameData, UINT32 FrameLength, UINT32 RxStatus)
{
 /* Received Frame Data Store Format */
 /* +------------------+             */
 /* | Preamble         |             */
 /* | Sequence         |             */
 /* | Status  | Length |             */
 /* | Rx Data          |             */
 /* +------------------+             */
#if 1
	if( m_pvRxCallBack != NULL ) 
		m_pvRxCallBack((UINT8 *)pFrameData, FrameLength);
#else
	UINT32 *rxFrameBuffer;
	if( m_pvRxCallBack == NULL ) return;

      rxFrameBuffer = (UINT32 *)m_RxFrameBasePtr;
	
	

    *rxFrameBuffer++ = RxFrameDataPreamble ;
    *rxFrameBuffer++ = gsMacRxStatus[num].RXGD;
    *rxFrameBuffer++ = (RxStatus << 16) | FrameLength  ;

    // Move Rx Frame to User Area
#ifdef RxInt_Msg    
  UART_printf("RxFrameBuffer=%x,pFrameData=%x\n",rxFrameBuffer,pFrameData);
#endif    
    memcpy ((UINT8 *)rxFrameBuffer,(UINT8 *)pFrameData,FrameLength);
	m_pvRxCallBack((UINT8 *)rxFrameBuffer, FrameLength);
#endif

}



// Interrupt Service Routine for MAC0 Tx
void MAC0_Tx_isr(void)
{   
 S_FrameDescriptor *pTxFDptr;
 UINT32 *pFrameDataPtr ;
 UINT32 Status, RdValue;
 UINT32 CTxPtr ;

 RdValue=MISTA_0;
 sysDisableInterrupt((INT_SOURCE_E)EMCTXINT0);

 if (RdValue & 0x00800000)
     gsMacTxStatus[0].TDU++;
     
 if (RdValue & 0x00020000)
     gsMacTxStatus[0].EMP++;
   
 //MISTA_0=RdValue&0xffff0000;   //change for MISTA writing problem

#ifdef TxInt_Msg
//if ((RdValue&0xffff0000)!=0x850000)
 UART_printf("MAC0_Tx_isr(%x), ",RdValue) ;
#endif 

 if (RdValue & MISTA_TxBErr)
   {
#ifdef TxInt_Msg
    UART_printf("TxBErr\n") ;
#endif
#ifdef _W55FA92_
    MCMDR_0 |= SWR;
#endif    
    LanInitialize(0);
    MacTxDoneFlagForLoopBackCheck[0] = 1 ;
    gsMacTxStatus[0].TxBErr++;
   }
 else
   {
    CTxPtr = CTXDSA_0 ;

    while ( gCTxFDPtr[0] != CTxPtr )
    {
     pTxFDptr = (S_FrameDescriptor *) gCTxFDPtr[0];

     // Check CPU ownership, if Owner is DMA then break
     pFrameDataPtr = (UINT32 *)&pTxFDptr->Status1;
     if ( (*pFrameDataPtr & TXfOwnership_DMA) )
       break ;

     Status = (pTxFDptr->Status2 >> 16) & 0xffff;
#ifdef TxInt_Msg     
 UART_printf("Tx_Status=%x\n",Status) ;
#endif 
     if (Status & TXFD_TXCP)
     {
       gsMacTxStatus[0].TXCP++ ;  
       gsMacTxStatus[0].TxBytes += pTxFDptr->Status2 & 0xFFFF; //CMN
     }
     else
     {
        gTxErrPacketCnt[0]++;
        // Save Error status
        if (Status & TXFD_TXABT) gsMacTxStatus[0].TXABT++ ;
        if (Status & TXFD_DEF)   gsMacTxStatus[0].DEF++ ;
        if (Status & TXFD_PAU)   gsMacTxStatus[0].PAU++ ;
        if (Status & TXFD_EXDEF) gsMacTxStatus[0].EXDEF++ ;
        if (Status & TXFD_NCS)   gsMacTxStatus[0].NCS++ ;
        if (Status & TXFD_SQE)   gsMacTxStatus[0].SQE++ ;
        if (Status & TXFD_LC)    gsMacTxStatus[0].LC++ ;
        if (Status & TXFD_TXHA)  gsMacTxStatus[0].TXHA++ ;
       }

     // Clear Framedata pointer already used.
     //pTxFDptr->Status2 = (UINT32)0x0000FFFF;

     gCTxFDPtr[0] = (UINT32)pTxFDptr->NextFrameDescriptor ;
    }
    
    MacTxDoneFlagForLoopBackCheck[0] = 1 ;
   }
}



// Interrupt Service Routine for MAC0 Rx
void MAC0_Rx_isr(void)
{
 S_FrameDescriptor *pRxFDptr ;
 UINT32 RxStatus, FrameLength ;
 UINT32 CRxPtr;
 UINT8 *pFrameData ;
 UINT32 RdValue;

 RdValue=MISTA_0;
 MISTA_0=RdValue; //&0x0000ffff;   //change for MISTA writing problem
 sysEnableInterrupt((INT_SOURCE_E)EMCTXINT0);

 if (RdValue & 0x400)
     gsMacRxStatus[0].RDU++;           
 
 
#ifdef RxInt_Msg 
 UART_printf("MAC0_Rx_isr(%x)\n",RdValue) ;
#endif 

 if (RdValue & MISTA_RXOV)
   {
#ifdef RxInt_Msg 
   	UART_printf("RXOV\n") ;
#endif   	
    gsMacRxStatus[0].RXOV++ ; 
   }

 
 if (RdValue & MISTA_RxBErr)
   {
#ifdef RxInt_Msg 
    UART_printf("RxBErr\n") ;
#endif    
#ifdef _W55FA92_
    MCMDR_0 |= SWR;
#endif    
    LanInitialize(0);
    MacRxDoneFlagForLoopBackCheck[0] = 1 ;
    gsMacRxStatus[0].RxBErr++ ; 
   }
 else if (RdValue & MISTA_CFR)
   {
#ifdef RxInt_Msg 
    UART_printf("CFR(%x)\n",MGSTA_0) ;   
#endif      
    gsMacRxStatus[0].CFR++ ;
    MacRxDoneFlagForLoopBackCheck[0] = 1 ; // only used for loopback test
    RSDR_0 = 0;    
   }
 else if (RdValue & MISTA_CRCE)
   {
#ifdef RxInt_Msg 
   	UART_printf("CRCE\n") ;
#endif   	
    gsMacRxStatus[0].CRCE++ ;
    MacRxDoneFlagForLoopBackCheck[0] = 2 ; // only used for loopback test
   }
 else if (RdValue & MISTA_PTLE)
   {
#ifdef RxInt_Msg 
   	UART_printf("PTLE\n") ;
#endif   	
    gsMacRxStatus[0].PTLE++ ;
    MacRxDoneFlagForLoopBackCheck[0] = 2 ; // only used for loopback test
   }
 else if (RdValue & MISTA_ALIE)
   {
#ifdef RxInt_Msg 
   	UART_printf("ALIE\n") ;
#endif   	
    gsMacRxStatus[0].ALIE++ ;
    MacRxDoneFlagForLoopBackCheck[0] = 2 ; // only used for loopback test
   }
 else if (RdValue & MISTA_RP)
   {
#ifdef RxInt_Msg 
   	UART_printf("RP\n") ;
#endif   	
    gsMacRxStatus[0].RP++ ;
    MacRxDoneFlagForLoopBackCheck[0] = 2 ; // only used for loopback test
   }
 else if (RdValue & MISTA_DFOI)
   {
#ifdef RxInt_Msg 
    UART_printf("DFO\n") ;   
#endif      
    DFOIflag=1 ;
    MacRxDoneFlagForLoopBackCheck[0] = 1 ; // only used for loopback test
   }
   
 if (RdValue & MISTA_RXOV)
   {
#ifdef RxInt_Msg 
   	UART_printf("RXOV\n") ;
#endif   	
    RXOVflag = 1 ; 
   }
    
 if (RdValue & MISTA_MMP)
   {
#ifdef RxInt_Msg 
   	UART_printf("MMP\n") ;
#endif   	
    gsMacRxStatus[0].MMP++ ;
    MacRxDoneFlagForLoopBackCheck[0] = 2 ; // only used for loopback test
   }  
   
 if (RdValue & MISTA_DENI)
   {
#ifdef RxInt_Msg 
   	UART_printf("DENI\n") ;
#endif   	
    DENIflag=1 ;
   }
 
// else
   {
    // Get current frame descriptor
    CRxPtr = CRXDSA_0 ;

    while (CRxPtr != gCRxFDPtr[0])
    {
     // Get Rx Frame Descriptor
     pRxFDptr = (S_FrameDescriptor *)gCRxFDPtr[0];

     if ((pRxFDptr->Status1|RXfOwnership_CPU)==RXfOwnership_CPU) // ownership=CPU
       {
        RxStatus = (pRxFDptr->Status1 >> 16) & 0xffff;
#ifdef RxInt_Msg        
 UART_printf("Rx_Status=%x, len=%d\n",RxStatus, (pRxFDptr->Status1 & 0xFFFF)) ;
#endif 
        // If Rx frame is good, then process received frame
        if(RxStatus & RXFD_RXGD)
        {
         FrameLength = pRxFDptr->Status1 & 0xffff ;         
         pFrameData = (UINT8 *)pRxFDptr->FrameDataPtr ;
         gsMacRxStatus[0].RXGD++ ;
		 gsMacRxStatus[0].RxBytes += FrameLength;
         // Get received frame to memory buffer
         GetRxFrameData(0, pFrameData, FrameLength, RxStatus) ;
         MacRxDoneFlagForLoopBackCheck[0] = 56 ; // only used for loopback test
        }
        else
        {
         // If Rx frame has error, then process error frame
         gRxErrPacketCnt[0]++ ;

         MacRxDoneFlagForLoopBackCheck[0] = 55 ; // only used for loopback test
        }
       }
     else
     {
       if (RdValue & MISTA_CRCE)
       		gRxCRCErrOnDMAPacketCnt[0]++;
       break;
      }     
     // Change ownership to DMA for next use
     pRxFDptr->Status1 |= RXfOwnership_DMA;

     // Get Next Frame Descriptor pointer to process
     gCRxFDPtr[0] = (UINT32)(pRxFDptr->NextFrameDescriptor) ;
    } 
        
    MacRxDoneFlagForLoopBackCheck[0] = 1 ; // only used for loopback test

    RSDR_0 = 0;
    
   }
 MISTA_0=RdValue&0x0000ffff; //CMN   
}




void CheckTxFDStatus(int port, S_FrameDescriptor *pTxFDptr)
{
   UINT32 Status;
   
   Status = (pTxFDptr->Status2 >> 16) & 0xffff;

   if (Status & TXFD_TXCP)
   {
       gsMacTxStatus[port].TXCP++ ;  
       gsMacTxStatus[port].TxBytes += pTxFDptr->Status2 & 0xFFFF;
       
       TxPktSeq = *((volatile UINT32 *) (pTxFDptr->FrameDataPtr + 16));
       if (TxPktSeq != TxPktSeqWanted)
       {
           *((volatile unsigned int *) 0xFFF03210) = 0x43;
           TxPktSeqErr++;
           TxPktSeqWanted = TxPktSeq+1;
       }
       else
           TxPktSeqWanted++;
       
   }
   else 
        gTxErrPacketCnt[port]++;
   if (Status & TXFD_TXABT) gsMacTxStatus[port].TXABT++ ;
       
   // Save Error status
   if (Status & TXFD_DEF)   gsMacTxStatus[port].DEF++ ;
   if (Status & TXFD_PAU)   gsMacTxStatus[port].PAU++ ;
   if (Status & TXFD_EXDEF) gsMacTxStatus[port].EXDEF++ ;
   if (Status & TXFD_NCS)   gsMacTxStatus[port].NCS++ ;
   if (Status & TXFD_SQE)   gsMacTxStatus[port].SQE++ ;
   if (Status & TXFD_LC)    gsMacTxStatus[port].LC++ ;
   if (Status & TXFD_TXHA)  gsMacTxStatus[port].TXHA++ ;
 
   pTxFDptr->Status2 &= (UINT32) 0x0000FFFF;
   gCTxFDPtr[port] = (UINT32)pTxFDptr->NextFrameDescriptor ;

}



// Send ethernet frame function
int SendPacket(int num, UINT8 *Data,int Size)
{
 S_FrameDescriptor *psTxFD;
 UINT32              *pFrameDataPtr ;
 UINT8               *pFrameData ;
 int              FrameLength ;
 UINT32              *pTXFDStatus1;

#ifdef TxInt_Msg        
// UART_printf("SendPacket\n") ;     
#endif

    // Get Tx frame descriptor & data pointer
    psTxFD = (S_FrameDescriptor *)gWTxFDPtr[0] ;

    pFrameData = (UINT8 *)psTxFD->FrameDataPtr ;
    pFrameDataPtr = (UINT32 *)&psTxFD->FrameDataPtr;
    pTXFDStatus1 = (UINT32 *)&psTxFD->Status1;
//    FrameLength = Size + sizeof(etheader) ;
	    FrameLength = Size;

    // Check DMA ownership
    if ( (*pTXFDStatus1 & TXfOwnership_DMA) ) 
    {
     UART_printf("ownership is DMA\n");
     return 0 ;
    }

    //CMN
    if (psTxFD->Status2 & 0xFFFF0000)  
    {
        CheckTxFDStatus(0, psTxFD);   //The descriptor hasn't been processed yet.
        DoChk++;
    }
#if 0 // PktSeq is only for HW verification 
    //if (EXTERNAL_LOOPBACK_PORT == 0)
        *((volatile UINT32 *) (Data + 16)) = PktSeq++;
#endif

    // Prepare Tx Frame data to Frame buffer
    memcpy ((UINT8 *)pFrameData,(UINT8 *)Data,FrameLength);

    // Set TX Frame flag & Length Field
    psTxFD->Status1 |= (PaddingMode | CRCMode | MACTxIntEn);
    psTxFD->Status2 = (UINT32)(FrameLength & 0xffff);

    // Cheange ownership to DMA
    psTxFD->Status1 |= TXfOwnership_DMA;

    // Enable MAC Tx control register
    MacTxGo(0);

    // Change the Tx frame descriptor for next use
    gWTxFDPtr[0] = (UINT32)(psTxFD->NextFrameDescriptor);


 return 1 ;
}


// Transfer Control Frame Data to another Host
void ControlPauseTransfer(int num,int timeslot)
{    

    // set destination address to CAM #13
    FillCamEntry(0, 13, PauseMultiAddr0, PauseMultiAddr1);

    // set source address to CAM #14
    FillCamEntry(0, 14, gCam0M_0, gCam0L_0);

    // set length, type, opcode, and operand to CAM #15
    FillCamEntry(0, 15, 0x88080001, (UINT32)timeslot<<16);

    // set Send Pause bit and Transmit On in MCMDR
    MCMDR_0 |= MCMDR_SDPZ ;

    // Wait control frame finished
    while ( (MCMDR_0 & MCMDR_SDPZ) ) ;
 
 //UART_printf("End ControlPauseTransfer\n") ;
}

//==================================================

BOOL EMAC_Init(NVT_EMAC_T *emac)
{

 // Multifun pin
	if( emac->portNo == 0 )	// LCM Data Bus pins
	{
		*((unsigned volatile int *)(GCR_BA+0x94)) = 0x44444444; 	// GPCFUN1: GPC15 ~GPC8[31:0] 

		*((unsigned volatile int *)(GCR_BA+0xA0)) &= ~0xFF; 	// GPEFUN0: GPE1~0[7:0]
		*((unsigned volatile int *)(GCR_BA+0xA0)) |= 0x44;
	} else {	// JTAG1 + TVOut + LVSYNC + LVDE pins

		*((unsigned volatile int *)(GCR_BA+0x98)) &= ~0xFFFF; 	// GPDFUN0: MF_GPD0~3[15:0]
		*((unsigned volatile int *)(GCR_BA+0x98)) |= 0xEEEE;
		*((unsigned volatile int *)(GCR_BA+0x9C)) &= ~0xFF00; 	// GPDFUN1: MF_GPD11[15:12] MF_GPD10[11:8]
		*((unsigned volatile int *)(GCR_BA+0x9C)) |= 0xEE00;	

		*((unsigned volatile int *)(GCR_BA+0xF4)) &= ~0x80000000;  	// Switch to digitial pin
	
		*((unsigned volatile int *)(GCR_BA+0xE8)) &= ~0xFFFF00; 	// GPGFUN0: GPG5~2[23:7]
		*((unsigned volatile int *)(GCR_BA+0xE8)) |= 0xEEEE00;
		*((unsigned volatile int *)(GPIO_BA+0x34)) &= ~0x0C0F;		//GPIOD_PUEN: disable PUEN GPIOD_PUEN[0~3, 10~11]=0
	}


 *((unsigned volatile int *) (CLK_BA+0x34) ) |= 0x80;	//AHBCLK3, enable EMAC CLK
 
 m_pvRxCallBack = NULL;

 MyMacSrcAddr[0][0] = emac->srcMAcAddr[0];
 MyMacSrcAddr[0][1] = emac->srcMAcAddr[1];
 MyMacSrcAddr[0][2] = emac->srcMAcAddr[2];
 MyMacSrcAddr[0][3] = emac->srcMAcAddr[3];
 MyMacSrcAddr[0][4] = emac->srcMAcAddr[4];
 MyMacSrcAddr[0][5] = emac->srcMAcAddr[5];

 m_mode = emac->speedMode;
 m_CAMCMR = emac->recvPacketType;
 m_RxFDBaseAddr0 = emac->rxFDBaseAddr;
 m_TxFDBaseAddr0 = emac->txFDBaseAddr;
 m_RxFBABaseAddr0 = emac->rxFBABaseAddr;
 m_TxFBABaseAddr0 = emac->txFBABaseAddr;

 LanInitialize(0);
 return TRUE;
}

BOOL EMAC_SetRxCallBack(PVOID pvFun)
{
	m_pvRxCallBack = (sys_pvFunPtr)pvFun;
	return TRUE;
}

BOOL EMAC_SendPacket(UINT8 *data,int size)
{
	return SendPacket(0,data,size);
}



void EMAC_EnableWakeOnLan()
{

 outpw(REG_CAMCMR, CAM_ECMP | CAM_AUP | CAM_AMP | CAM_ABP);
 outpw(REG_MCMDR, inpw(REG_MCMDR)|0x40); // Enable EMAC wake up feature
 outpw(GCR_BA+0x34, inpw(GCR_BA+0x34)|0x40);	// Enable EMAC as one wake up source

}


void EMAC_Exit()
{

    // Disable MAC Tx and Rx interrupt.
  sysDisableInterrupt((INT_SOURCE_E)EMCTXINT0);
  sysDisableInterrupt((INT_SOURCE_E)EMCRXINT0);

 *((unsigned volatile int *) (CLK_BA+0x34)) &= ~(0x80); //AHBCLK3, disable EMAC CLK

}


