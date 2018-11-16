/****************************************************************************
* Copyright (c) 2012-2015 Nuvoton Electronics Corp.
* All rights reserved.
*
* FILE NAME: mac.h
*
* DESCRIPTION: Header File for MAC.
*
****************************************************************************/
#ifndef W55FA92_MAC_LIB_H
#define W55FA92_MAC_LIB_H

#include "w55fa92_reg.h"	
#include "wbtypes.h"

//#define UART_printf printf 
//#define TxInt_Msg
//#define RxInt_Msg


/****************************************************************************************************
 *                                                               
 * I/O routines  
 *
 ****************************************************************************************************/
#define VPint   			*(unsigned int volatile *)
#define VPshort 			*(unsigned short volatile *)
#define VPchar  			*(unsigned char volatile *)



#define 	EMCTXINT0	36	/* EMC TX Interrupt no */
#define 	EMCRXINT0	37	/* EMC RX Interrupt no */

///////////////////////////////////////////////////////////////

#define CAMCMR_0       (VPint(REG_CAMCMR))   /* CAM Command Register */
#define CAMEN_0        (VPint(REG_CAMEN))   /* CAM Enable Register */
#define CAM0M_Base_0   (EMAC_BA+0x08)
#define CAM0L_Base_0   (EMAC_BA+0x0c)
#define CAMxM_Reg_0(x) (VPint(REG_CAMxM_Reg(x)))   /*  */
#define CAMxL_Reg_0(x) (VPint(REG_CAMxL_Reg(x)))   /*  */

#define TXDLSA_0       (VPint(REG_TXDLSA))   /* Transmit Descriptor Link List Start Address Register */
#define RXDLSA_0       (VPint(REG_RXDLSA))   /* Receive Descriptor Link List Start Address Register */
#define MCMDR_0        (VPint(REG_MCMDR))   /* MAC Command Register */
#define MIID_0         (VPint(REG_MIID))   /* MII Management Data Register */
#define MIIDA_0        (VPint(REG_MIIDA))   /* MII Management Control and Address Register */
#define FFTCR_0		   (VPint(REG_FFTCR))   /* FIFO Threshold Control Register */
#define TSDR_0         (VPint(REG_TSDR))   /* Transmit Start Demand Register */
#define RSDR_0         (VPint(REG_RSDR))   /* Receive Start Demand Register */
#define DMARFC_0	   (VPint(REG_DMARFC))   /* Maximum Receive Frame Control Register */
#define MIEN_0         (VPint(REG_MIEN))   /* MAC Interrupt Enable Register */
          /* Status Registers */
#define MISTA_0        (VPint(REG_MISTA))   /* MAC Interrupt Status Register */
#define MGSTA_0        (VPint(REG_MGSTA))   /* MAC General Status Register */
#define MPCNT_0		   (VPint(REG_MPCNT))   /* Missed Packet Count Register */
#define MRPC_0         (VPint(REG_MRPC))   /* MAC Receive Pause Count Register */
#define MRPCC_0        (VPint(REG_MRPCC))   /* MAC Receive Pause Current Count Register */
#define MREPC_0        (VPint(REG_MREPC))   /* MAC Remote Pause Count Register */
#define DMARFS_0       (VPint(REG_DMARFS))   /* DMA Receive Frame Status Register */
#define CTXDSA_0       (VPint(REG_CTXDSA))   /* Current Transmit Descriptor Start Address Register */
#define CTXBSA_0       (VPint(REG_CTXBSA))   /* Current Transmit Buffer Start Address Register */
#define CRXDSA_0       (VPint(REG_CRXDSA))   /* Current Receive Descriptor Start Address Register */
#define CRXBSA_0       (VPint(REG_CRXBSA))   /* Current Receive Buffer Start Address Register */
          /* Diagnostic Registers */
#define RXFSM_0        (VPint(REG_RXFSM))   /* Receive Finite State Machine Register */
#define TXFSM_0        (VPint(REG_TXFSM))   /* Transmit Finite State Machine Register */
#define FSM0_0         (VPint(REG_FSM0))   /* Finite State Machine Register 0 */
#define FSM1_0         (VPint(REG_FSM1))   /* Finite State Machine Register 1 */
#define DCR_0          (VPint(REG_DCR))   /* Debug Configuration Register */
#define DMMIR_0		   (VPint(REG_DMMIR))   /* Debug Mode MAC Information Register */
#define BISTR_0        (VPint(REG_BISTR))   /* BIST Mode Register */


/////////////////////////////////////////////////////////////////

#define MacReg(num,x)    (VPint(0xB100E000+num*0x800+x*0x4))
#define MacReg_DD(num,x) (VPint(0xB100E200+num*0x800+x*0x4))


//#define PHY_Auto_Negotiation
	
#define MODE_DR100_FULL   3
#define MODE_DR100_HALF   2
#define MODE_DR10_FULL    1
#define MODE_DR10_HALF    0

#define MAC_WAN 1
#define MAC_LAN 0

#define MAC_NUM 0

// CAM Command Register(CAMCMR)
#define CAM_AUP  0x0001  // Accept Packets with Unicast Address
#define CAM_AMP  0x0002  // Accept Packets with Multicast Address
#define CAM_ABP  0x0004  // Accept Packets with Broadcast Address
#define CAM_CCAM 0x0008  // 0: Accept Packets CAM Recognizes and Reject Others
                         // 1: Reject Packets CAM Recognizes and Accept Others
#define CAM_ECMP 0x0010  // Enable CAM Compare


// MAC Interrupt Enable Register(MIEN)
#define EnRXINTR 0x00000001  // Enable Interrupt on Receive Interrupt
#define EnCRCE   0x00000002  // Enable CRC Error Interrupt
#define EnRXOV   0x00000004  // Enable Receive FIFO Overflow Interrupt
#define EnPTLE   0x00000008  // Enable Packet Too Long Interrupt
#define EnRXGD   0x00000010  // Enable Receive Good Interrupt
#define EnALIE   0x00000020  // Enable Alignment Error Interrupt
#define EnRP     0x00000040  // Enable Runt Packet on Receive Interrupt
#define EnMMP    0x00000080  // Enable More Missed Packets Interrupt
#define EnDFO    0x00000100  // Enable DMA receive frame over maximum size Interrupt
#define EnDEN    0x00000200  // Enable DMA early notification Interrupt
#define EnRDU    0x00000400  // Enable Receive Descriptor Unavailable Interrupt
#define EnRxBErr 0x00000800  // Enable Receive Bus ERROR interrupt
#define EnNATOK  0x00001000  // Enable NAT Processing OK Interrupt
#define EnNATErr 0x00002000  // Enable NAT Processing Error Interrupt
#define EnCFR    0x00004000  // Enable Control Frame Receive Interrupt
#define EnTXINTR 0x00010000  // Enable Interrupt on Transmit Interrupt
#define EnTXEMP  0x00020000  // Enable Transmit FIFO Empty Interrupt
#define EnTXCP   0x00040000  // Enable Transmit Completion Interrupt
#define EnEXDEF  0x00080000  // Enable Defer Interrupt
#define EnNCS    0x00100000  // Enable No Carrier Sense Interrupt
#define EnTXABT  0x00200000  // Enable Transmit Abort Interrupt
#define EnLC     0x00400000  // Enable Late Collision Interrupt
#define EnTDU    0x00800000  // Enable Transmit Descriptor Unavailable Interrupt
#define EnTxBErr 0x01000000  // Enable Transmit Bus ERROR Interrupt


// MAC Command Register(MCMDR)
#define MCMDR_RXON    0x00000001  // Receive ON
#define MCMDR_ALP     0x00000002  // Accept Long Packet
#define MCMDR_ARP     0x00000004  // Accept Runt Packet
#define MCMDR_ACP     0x00000008  // Accept Control Packet
#define MCMDR_AEP     0x00000010  // Accept Error Packet
#define MCMDR_SPCRC   0x00000020  // Accept Strip CRC Value
#define MCMDR_TXON    0x00000100  // Transmit On
#define MCMDR_NDEF    0x00000200  // No defer
#define MCMDR_SDPZ    0x00010000  // Send Pause
#define MCMDR_EnSQE   0x00020000  // Enable SQE test
#define MCMDR_FDUP    0x00040000  // Full Duplex
#define MCMDR_EnMDC   0x00080000  // Enable MDC signal
#define MCMDR_OPMOD   0x00100000  // Operation Mode
#define MCMDR_LBK     0x00200000  // Loop Back
#define MCMDR_EnRMII  0x00400000  // Enable RMII
#define MCMDR_LAN     0x00800000  // LAN Port Setting Mode
#define SWR        	  0x01000000  // Software Reset

// MAC MII Management Data Control and Address Register(MIIDA)
//#define MDCCR    0x00000000  // MDC clock rating
#define MDCCR    0x00a00000  // MDC clock rating // MDCON
//#define PHYAD    0x00000000  // PHY Address
//#define PHYAD    0x00000200  // PHY Address
#define PHYWR    0x00010000  // Write Operation
#define PHYBUSY  0x00020000  // Busy Bit
#define PHYPreSP 0x00040000  // Preamble Suppress

extern volatile unsigned int PHYAD;

// FIFO Threshold Adjustment Register(FIFOTHD)

#define TxTHD_1    0x00000100  // 1/4 Transmit FIFO Threshold
#define TxTHD_2    0x00000200  // 2/4 Transmit FIFO Threshold
#define TxTHD_3    0x00000300  // 3/4 Transmit FIFO Threshold
#define RxTHD_1    0x00000001  // 1/4 Receive FIFO Threshold
#define RxTHD_2    0x00000002  // 2/4 Receive FIFO Threshold
#define RxTHD_3    0x00000003  // 3/4 Receive FIFO Threshold
#define Blength_8  0x00100000  // DMA burst length 8 beats
#define Blength_12 0x00200000  // DMA burst length 12 beats
#define Blength_16 0x00300000  // DMA burst length 16 beats


// MAC Interrupt Status Register(MISTA)
#define MISTA_RXINTR 0x00000001  // Interrupt on Receive
#define MISTA_CRCE   0x00000002  // CRC Error
#define MISTA_RXOV   0x00000004  // Receive FIFO Overflow error
#define MISTA_PTLE   0x00000008  // Packet Too Long Error
#define MISTA_RXGD   0x00000010  // Receive Good
#define MISTA_ALIE   0x00000020  // Alignment Error
#define MISTA_RP     0x00000040  // Runt Packet
#define MISTA_MMP    0x00000080  // More Missed Packets than miss rolling over counter flag
#define MISTA_DFOI   0x00000100  // DMA receive frame over maximum size interrupt
#define MISTA_DENI   0x00000200  // DMA early notification interrupt
#define MISTA_RDU    0x00000400  // Receive Descriptor Unavailable interrupt
#define MISTA_RxBErr 0x00000800  // Receive Bus Error interrupt
#define MISTA_NATOK  0x00001000  // NAT Processing OK
#define MISTA_NATErr 0x00002000  // NAT Processing Error
#define MISTA_CFR    0x00004000  // Control Frame Receive
#define MISTA_TXINTR 0x00010000  // Interrupt on Transmit
#define MISTA_TXEMP  0x00020000  // Transmit FIFO Empty
#define MISTA_TXCP   0x00040000  // Transmit Completion
#define MISTA_EXDEF  0x00080000  // Defer
#define MISTA_NCS    0x00100000  // No Carrier Sense
#define MISTA_TXABT  0x00200000  // Transmit Abort
#define MISTA_LC     0x00400000  // Late Collision
#define MISTA_TDU    0x00800000  // Transmit Descriptor Unavailable interrupt
#define MISTA_TxBErr 0x01000000  // Transmit Bus Error interrupt


// MAC General Status Register(MGSTA)
#define MGSTA_CFR  0x00000001  // Control Frame Received
#define MGSTA_RXHA 0x00000002  // Reception Halted
#define MGSTA_RFFull 0x00000004 //RxFIFO is full 
#define MGSTA_DEF  0x00000010  // Deferred transmission
#define MGSTA_PAU  0x00000020  // Pause Bit
#define MGSTA_SQE  0x00000040  // Signal Quality Error
#define MGSTA_TXHA 0x00000080  // Transmission Halted

// BIST Mode Register(BISTR)
#define BISTR_BMEn     0x00000001  // BIST Mode Enable
#define BISTR_Finish   0x00000002  // BIST Operation Finish
#define BISTR_BistFail 0xfffffff3  // BIST Fail

// PHY(DM9161) Register Description
#define PHY_CNTL_REG    0x00
#define PHY_STATUS_REG  0x01
#define PHY_ID1_REG     0x02
#define PHY_ID2_REG     0x03
#define PHY_ANA_REG     0x04
#define PHY_ANLPA_REG   0x05
#define PHY_ANE_REG     0x06

#define PHY_DSC_REG     0x10
#define PHY_DSCS_REG    0x11
#define PHY_10BTCS_REG  0x12
#define PHY_SINT_REG    0x15
#define PHY_SREC_REG    0x16
#define PHY_DISC_REG    0x17

//PHY Control Register
#define RESET_PHY       1 << 15
#define ENABLE_LOOPBACK 1 << 14
#define DR_100MB        1 << 13
#define ENABLE_AN       1 << 12
#define PHY_POWER_DOWN  1 << 11
#define PHY_MAC_ISOLATE 1 << 10
#define RESTART_AN      1 << 9
#define PHY_FULLDUPLEX  1 << 8
#define PHY_COL_TEST    1 << 7

// PHY Status Register
#define AN_COMPLETE     1 << 5

// PHY Auto-negotiation Advertisement Register
#define DR100_TX_FULL   1 << 8
#define DR100_TX_HALF   1 << 7
#define DR10_TX_FULL    1 << 6
#define DR10_TX_HALF    1 << 5
#define IEEE_802_3_CSMA_CD   1


#define IP_ADDR_SIZE          4
#define MAC_ADDR_SIZE         6

#define MaxRxFrameSize        1520 // Rx Frame Max Size = 1520
//
#define MaxRxFrameDescriptors 32   // Max number of Rx Frame Descriptors
#define MaxTxFrameDescriptors 16   // Max number of Tx Frame Descriptors
#define MaxRxFrameData        32   // Max number of Rx Frame Data
#define MaxTxFrameData        16   // Max number of Tx Frame Data
//


// RX Frame Descriptor's Owner bit
#define RXfOwnership_DMA 0x80000000  // 10 = DMA
#define RXfOwnership_NAT 0xc0000000  // 11 = NAT
#define RXfOwnership_CPU 0x3fffffff  // 00 = CPU

// TX Frame Descriptor's Owner bit
#define TXfOwnership_DMA 0x80000000  // 1 = DMA
#define TXfOwnership_CPU 0x7fffffff  // 0 = CPU


// Rx Frame Descriptor Status
#define RXFD_Hit     0x2000  // current packet is hit with NAT entry table
#define RXFD_IPHit   0x1000  // current packet is hit on IP address
#define RXFD_PortHit 0x0800  // current packet is hit on Port Number
#define RXFD_Inverse 0x0400  // current hit entry is setting on inverse mode
#define RXFD_NATFSH  0x0200  // NAT Processing Finish
//#define RXFD_NATFSH  0x0000  // cmn for debugging !!! [05/29]
#define RXFD_RP      0x0040  // Runt Packet
#define RXFD_ALIE    0x0020  // Alignment Error
#define RXFD_RXGD    0x0010  // Receiving Good packet received
#define RXFD_PTLE    0x0008  // Packet Too Long Error
#define RXFD_CRCE    0x0002  // CRC Error
#define RXFD_RXINTR  0x0001  // Interrupt on receive

// Rx Frame Descriptor NAT Information
#define RXFD_NAT_URG     0x20000000  // TCP status(URG)
#define RXFD_NAT_ACK     0x10000000  // TCP status(ACK)
#define RXFD_NAT_PSH     0x08000000  // TCP status(PSH)
#define RXFD_NAT_RST     0x04000000  // TCP status(RST)
#define RXFD_NAT_SYN     0x02000000  // TCP status(SYN)
#define RXFD_NAT_FIN     0x01000000  // TCP status(FIN)
#define RXFD_NAT_UCKERR  0x00400000  // TCP/UCKS Error
#define RXFD_NAT_TUERR   0x00200000  // TCP/UDP Error
#define RXFD_NAT_NHERR   0x00100000  // No Hit Error
#define RXFD_NAT_PPPCaps 0x00000040  // PPPoE datagram encapsulated
#define RXFD_NAT_PPPoE   0x00000020  // PPPoE protocol
#define RXFD_NAT_UCKS    0x00000010  // UDP protocol with skip checksum replacement
#define RXFD_NAT_UDP     0x00000008  // apply UDP protocol
#define RXFD_NAT_TCP     0x00000004  // apply TCP protocol
#define RXFD_NAT_LAN     0x00000002  // internal (LAN) port gets hit
#define RXFD_NAT_Hit     0x00000001  // current packet is hit with NAT entry table


// Tx Frame Descriptor's Control bits
#define MACTxIntEn    0x04
#define CRCMode       0x02
#define NoCRCMode     0x00
#define PaddingMode   0x01
#define NoPaddingMode 0x00

// Tx Frame Descriptor Status
#define TXFD_TXINTR 0x0001  // Interrupt on Transmit
#define TXFD_DEF    0x0002  // Transmit deferred 
#define TXFD_TXCP   0x0008  // Transmission Completion 
#define TXFD_EXDEF  0x0010  // Exceed Deferral
#define TXFD_NCS    0x0020  // No Carrier Sense Error
#define TXFD_TXABT  0x0040  // Transmission Abort 
#define TXFD_LC     0x0080  // Late Collision 
#define TXFD_TXHA   0x0100  // Transmission halted
#define TXFD_PAU    0x0200  // Paused
#define TXFD_SQE    0x0400  // SQE error 


// Tx/Rx buffer descriptor structure
typedef struct FrameDescriptor
{
 UINT32 Status1; // RX - Ownership(2bits),RxStatus(14bits),Length(16bits)
              // TX - Ownership(1bits),Control bits(3bits),Reserved(28bits)
 UINT32 FrameDataPtr;
 UINT32 Status2; // RX - NAT Information/Reserved(32bits)
              // TX - TxStatus(16bits),Length(16bits)
 UINT32 NextFrameDescriptor;
} S_FrameDescriptor;


// MAC Frame Structure
typedef struct ETH_HEADER
{
 UINT8 DestinationAddr[6] ;
 UINT8 SourceAddr[6] ;
 UINT8 LengthOrType[2] ;
} S_Etheader ;

typedef struct MACFrame
{
 S_Etheader Header ;
 UINT8       LLCData[1522];

} S_MACFrame;


// MAC Tx Error Structure
typedef struct MACTxStatus
{
 UINT32 DEF;
 UINT32 TXCP;
 UINT32 EXDEF;
 UINT32 NCS;
 UINT32 TXABT;
 UINT32 LC;
 UINT32 TXHA;
 UINT32 PAU;
 UINT32 SQE;
 UINT32 TxBErr; //CMN [2002/11/01]
 UINT32 TxBytes;
 UINT32 TDU;
 UINT32 EMP;
} S_MACTxStatus;

// MAC Rx Error Structure
typedef struct MACRxStatus
{
 UINT32 RP;
 UINT32 ALIE;
 UINT32 RXGD;
 UINT32 PTLE;
 UINT32 CRCE;
 UINT32 CFR;
 UINT32 MMP;
 UINT32 RxBErr; //CMN [2002/11/01]
 UINT32 RxBytes;
 UINT32 RXOV;
 UINT32 RDU;
} S_MACRxStatus;


typedef struct NVT_EMAC_STRUCT_tag
{
		UINT8 		srcMAcAddr[6];
		UINT32		speedMode;
    	UINT32		recvPacketType;
    	UINT32		rxFDBaseAddr;
    	UINT32 		txFDBaseAddr;
    	UINT32 		rxFBABaseAddr;
    	UINT32 		txFBABaseAddr;
    	UINT32 		portNo;
}NVT_EMAC_T;


BOOL EMAC_Init(NVT_EMAC_T *emac);
BOOL EMAC_SetRxCallBack(PVOID pvFun);
BOOL EMAC_SendPacket(UINT8 *data,int size);
void EMAC_EnableWakeOnLan(void);
void EMAC_Exit(void);

#endif /* _MAC_LIB_ */
