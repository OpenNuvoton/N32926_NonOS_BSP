/**************************************************************************//**
 * @file     mscd.c
 * @version  V3.00
 * @brief    N3292 series Mass Storage Device driver source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"
#include "W55FA92_reg.h"
#include "usbd.h"
#include "NVTFAT.h"
#include "mass_storage_class.h"
#include "mscd.h"

#define USB_VID		0x0416		/* Vendor ID */ 
#define USB_PID		0x0092		/* Product ID */

#define DATA_CODE	"20230109"

/* RAM Disk size define : RAMDISK_1M ~ RAMDISK_16M */
#define RAMDISK_SIZE	MSC_RAMDISK_8M

#define __ARRAY_BUFFER__

#define MSC_BUFFER_SECTOR 256
#ifdef __ARRAY_BUFFER__
#if defined (__GNUC__)
UINT8 MSC_DATA_BUFFER[MSC_BUFFER_SECTOR * 512]  __attribute__((aligned(64)));
UINT8 MSC_CMD_BUFFER[4096]  __attribute__((aligned(64)));
#else
__align(64)	UINT8 MSC_DATA_BUFFER[MSC_BUFFER_SECTOR * 512];
__align(64)	UINT8 MSC_CMD_BUFFER[4096];
#endif
#endif

UINT8 Flash_Identify(UINT8 tLUN);
extern INT32 volatile usb_halt_ep;
#ifdef TEST_CDROM
PFN_MSCD_CDROM_CALLBACK pfnMSC_CDROM_CALLBACK;
VOID mscdCommand_43(void);
void mscdCDRead(UINT32 lba, UINT32 len);
void mscdCommand_46(void);
VOID mscdCommand_51(void);
VOID mscdCommand_4A(void);
VOID mscdCommand_A4(void);
void mscdModeSense_Command_CDROM(void);
BOOL volatile bFirstCMD = 0xFF, bIsDeviceReady = FALSE;
extern UINT8 CD_Tracks[];
#endif
#ifdef TEST_SD
#include "W55FA92_SIC.h"
#endif

#ifdef TEST_SM
#include "W55FA92_GNAND.h"
#include "W55FA92_SIC.h"
NDISK_T *ptMassNDisk;
NDISK_T *ptMassNDisk1;
NDISK_T *ptMassNDisk2;
#endif

typedef struct {
	UINT32	dCBWSignature; 
	UINT8	dCBWTag[4];
	union
	{
		UINT8	c[4];
		UINT32	c32;
	} dCBWDataTferLh;         
	UINT8	bmCBWFlags;	
	UINT8	bCBWLUN;	
	UINT8	bCBWCBLength;
	UINT8	OP_Code;
	UINT8	LUN;
	union 
	{
		UINT8	d[4];
		__packed	UINT32	d32;
	} LBA;
	union
	{
		UINT8	f[4];
		__packed	UINT32	f32;
	} CBWCB_In; 
	UINT8	CBWCB_Reserved[6];
} CBW;

union {
	CBW		CBWD; 	
	UINT8	CBW_Array[31];
} CBW_In;     

typedef struct {
	UINT8	dCSWSignature[4];
	UINT8	dCSWTag[4];
	union
	{
		UINT8	g[4];
		UINT32	g32;
	} dCSWDataResidue;    
	UINT8	bCSWStatus;	
} CSW;

union {
	CSW		CSWD; 	
	UINT8	CSW_Array[13];
} CSW_In;

/* Mass_Storage command base address */
extern volatile USBD_INFO_T usbdInfo;
volatile UINT8 g_mscd_block_mode = 1;
#ifdef TEST_SD
BOOL volatile bFirstInit0 = TRUE;
BOOL volatile bFirstInit1 = TRUE;
BOOL volatile bFirstInit2 = TRUE;
UINT32 volatile u32SdWriteProtectEnable = MSC_SD_PORT0;
UINT32 volatile u32SdUserWriteProtect = 0;
UINT32 volatile u32Sd0UserWriteProtectPort = 0;
UINT32 volatile u32Sd1UserWriteProtectPort = 0;
UINT32 volatile u32Sd2UserWriteProtectPort = 0;
UINT32 volatile u32Sd0UserWriteProtectPin = 0;
UINT32 volatile u32Sd1UserWriteProtectPin = 0;
UINT32 volatile u32Sd2UserWriteProtectPin = 0;
UINT32 volatile u32SdCardDetectEnable = MSC_SD_PORT0;
UINT32 volatile u32SdUserCardDetect = 0;
UINT32 volatile u32Sd0UserCardDetectPort = 0;
UINT32 volatile u32Sd1UserCardDetectPort = 0;
UINT32 volatile u32Sd2UserCardDetectPort = 0;
UINT32 volatile u32Sd0UserCardDetectPin = 0;
UINT32 volatile u32Sd1UserCardDetectPin = 0;
UINT32 volatile u32Sd2UserCardDetectPin = 0;
#endif
UINT32 volatile g_MSC_SD_PORT_ENABLE = MSC_SD_MP_PORT0;
UINT32 volatile g_MSC_NAND_CS_ENABLE = MSC_NAND_CS0;

/* USB Device Property */
extern USB_CMD_T	_usb_cmd_pkt;

/* MSC Device Property */
#if defined (__GNUC__)
volatile MSC_INFO_T mscdInfo  __attribute__((aligned(4))) = {0};
#else
__align(4) volatile MSC_INFO_T mscdInfo = {0};
#endif
/* MSC Descriptor */
#if defined (__GNUC__)
UINT8 MSC_DeviceDescriptor[MSC_DEVICE_DSCPT_LEN]  __attribute__((aligned(4))) =
#else
__align(4) UINT8 MSC_DeviceDescriptor[MSC_DEVICE_DSCPT_LEN] =
#endif
{
	MSC_DEVICE_DSCPT_LEN,
	0x01,
	0x00, 0x02,			/* bcdUSB - USB 2.0 */
	0x00,
	0x00,
	0x00,
	0x40,				/* bMaxPacketSize0 - 64 Bytes  */
	(USB_VID &0xFF) , ((USB_VID >> 8) & 0xFF) ,			/* Vendor ID */ 
	(USB_PID &0xFF) , ((USB_PID >> 8) & 0xFF),			/* Product ID */
	0x00, 0x11,			
	0x01,				/* iManufacture */
	0x02,				/* iProduct */
	0x03,				/* iSerialNumber */
	0x01				/* bNumConfigurations */
};

#if defined (__GNUC__)
static UINT32 MSC_QualifierDescriptor[3]  __attribute__((aligned(4))) =
#else
__align(4) static UINT32 MSC_QualifierDescriptor[3] = 
#endif
{
	0x0200060a, 0x40000000, 0x00000001
};

#if defined (__GNUC__)
static UINT32 MSC_ConfigurationBlock[8]  __attribute__((aligned(4))) =
#else
__align(4) static UINT32 MSC_ConfigurationBlock[8] =
#endif
{
	0x00200209, 0xC0000101, 0x00040932, 0x06080200, 0x05070050, 
	0x02000281, 0x020507FF, 0xFF020002
};
#if defined (__GNUC__)
static UINT32 MSC_ConfigurationBlockFull[8]  __attribute__((aligned(4))) =
#else
__align(4) static UINT32 MSC_ConfigurationBlockFull[8] =
#endif
{
	0x00200209, 0xC0000101, 0x00040932, 0x06080200, 0x05070050, 
	0x00400281, 0x020507FF, 0xFF004002
};

#if defined (__GNUC__)
static UINT32 MSC_HOSConfigurationBlock[8]  __attribute__((aligned(4))) =
#else
__align(4) static UINT32 MSC_HOSConfigurationBlock[8] =
#endif
{
	0x00200709, 0xC0000101, 0x00040932, 0x06080200, 0x05070050, 
	0x00400281, 0x020507FF, 0xFF004002
};

#if defined (__GNUC__)
static UINT32 MSC_FOSConfigurationBlock[8]  __attribute__((aligned(4))) =
#else
__align(4) static UINT32 MSC_FOSConfigurationBlock[8] =
#endif
{
	0x00200709, 0xC0000101, 0x00040932, 0x06080200, 0x05070050, 
	0x02000281, 0x020507FF, 0xFF020002
};

/* Identifier Language */
#if defined (__GNUC__)
static UINT32 MSC_StringDescriptor0[1]  __attribute__((aligned(4))) =
#else
__align(4) static UINT32 MSC_StringDescriptor0[1] = 
#endif
{
	LANGID_English_UnitedStates
};

/* iManufacturer */
#if defined (__GNUC__)
UINT8 MSC_StringDescriptor1[]  __attribute__((aligned(4))) =
#else
__align(4) UINT8 MSC_StringDescriptor1[] = 
#endif
{
	0x10,				 	/* bLength (Dafault Value is 0x10, the value will be set to actual value according to the Descriptor size wehn calling mscdInit) */
	0x03,					/* bDescriptorType */
	'n', 0, 'u', 0,	'v', 0, 'o', 0,	'T', 0, 'o', 0,	'n', 0
};

/* iProduct */
#if defined (__GNUC__)
UINT8 MSC_StringDescriptor2[]  __attribute__((aligned(4))) =
#else
__align(4) UINT8 MSC_StringDescriptor2[] = 
#endif
{
	0x10,				 	/* bLength (Dafault Value is 0x10, the value will be set to actual value according to the Descriptor size wehn calling mscdInit) */
	0x03,					/* bDescriptorType */
	'U', 0, 'S', 0,	'B', 0, ' ', 0,	'M', 0, 'S', 0,	'C', 0
};

/* iSerialNumber */
#if defined (__GNUC__)
UINT8 MSC_StringDescriptor3[]  __attribute__((aligned(4))) =
#else
__align(4) UINT8 MSC_StringDescriptor3[] = 
#endif
{
	0x1A,				 	/* bLength (Dafault Value is 0x1A, the value will be set to actual value according to the Descriptor size wehn calling mscdInit) */
	0x03,					/* bDescriptorType */
	'0', 0, '0', 0,	'0', 0, '0', 0,	'0', 0, '0', 0,	'5', 0, '5', 0,	'F', 0, 'A', 0,	'9', 0, '2', 0	
};

UINT32 volatile g_u32MscStorageAddr, g_u32MscMassAddr;

#ifdef TEST_SD
UINT32 volatile card_insert_flag0 = 0,card_eject_flag0 = 0;
UINT32 volatile card_insert_flag1 = 0,card_eject_flag1 = 0;
UINT32 volatile card_insert_flag2 = 0,card_eject_flag2 = 0;
UINT32 volatile g_ShowWriteProtectFlag0 = 1;
UINT32 volatile g_ShowWriteProtectFlag1 = 1;
UINT32 volatile g_ShowWriteProtectFlag2 = 1;
#endif

#ifdef TEST_RAM
void  DRAM_Identify(UINT8 cap);
UINT8 format(void);
UINT8 WriteBootSector(UINT8 fat16);
void  put_uint32(UINT32 value, UINT8 **p);
void  put_uint16(UINT16 value, UINT8 **p);
UINT8 Write_Sector(UINT32 sector,UINT8 *buffer);
UINT32 volatile g_u32MSCRamDiskAddr;

/* RAM Disk */
UINT8 StartSect[12] = {0,0x3b,0x1b,0x19,0x29,0x23,0x37,0x2f,0x5f,0xe1,0xe3,0xe3};
UINT8 PartBootSector[0x13] = {
		0xe9,0x00,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
		0x00,0x02,
		0x00,
		0x01,0x00,
		0x02,
		0x00,0x01,
	};

UINT8 FATSects[12] = {0,10,2,3,3,6,12,32,64,127,254,254};
UINT8 Flash_Buffer[512];
#endif

/* code = 12h, Inquiry */
#if defined (__GNUC__)
static UINT8 InquiryID[36]  __attribute__((aligned(4))) = {
#else
__align(4) static UINT8 InquiryID[36] = {
#endif
/* Direct-access device */
	0x00, 
/* Removable Media Bit */
	0x00,					// RMB
/* ISO version & ECMA version & ANSI Version */ 
	0x00,
/* Response Data Format */
	0x01,
/* Additional Length */	
	0x1F,
/* Reserved */	
	0x00, 0x00, 0x00,

/* Vendor Information (Byte8~15) */
	 	'W', '5', '5', 'F', 'A', '9', '5', ' ',
/* Product Identification (Byte 16~31) */
	 	'U', 'S', 'B', ' ', 'C', 'a', 'r', 'd',  
		' ', 'R', 'e', 'a', 'd', 'e', 'r', ' ',
		'1', '.', '0', '0'
};

/* Read-Write Error Recovery Page */
#if defined (__GNUC__)
static UINT8 Mode_Page_01[12]  __attribute__((aligned(4))) = {
#else
__align(4) static UINT8 Mode_Page_01[12] = {
#endif
/* Page code (Fixed) */
	0x01,
/* Page Length (Fixed) */	
	0x0A,
/* Error Recovery Parameters */	
	0x00,
/* Read Retry Count*/	
	0x03,
/* Reserved */	
	0x00, 0x00, 0x00, 0x00,
/* Write Retry Count */	
	0x03,
/* Reserved */		
	0x00, 0x00, 0x00 };

/* Flexible Disk Page */
#if defined (__GNUC__)
static UINT8 Mode_Page_05[32]  __attribute__((aligned(4))) = {
#else
__align(4) static UINT8 Mode_Page_05[32] = {
#endif
/* Page code (Fixed) */
	0x05,
/* Page Length (Fixed) */	
	0x1E,
/* Transfer Rate (Fixed) */	
	/* MSB, LSB */
	0x13, 0x88,	/* 5mbit/s transfer rate */
/* Number of Heads */	
	0x08,
/* Sector per Track */	
	0x20,
/* Data Bytes per Sector */	
	/* MSB, LSB */
	0x02, 0x00,
/* Number of Cylinders */	
	/* MSB, LSB */
	0x01, 0xF4,
/* Reserved */	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 
/* Motor On Delay (Fixed) */	
	0x05,   /* 0.5 Sec */
/* Motor Off Delay (Fixed) */	
	0x1E,	/* 3 Sec */
/* Reserved */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* Medium Rotation Rate (Fixed) */	
	/* MSB, LSB */
	0x01, 0x68,  /* 300 or 360 */
/* Reserved */	
	0x00, 0x00
};

/* Removable Block Access Capabilities Page */
#if defined (__GNUC__)
static UINT8 Mode_Page_1B[12]  __attribute__((aligned(4))) = {
#else
__align(4) static UINT8 Mode_Page_1B[12] = {
#endif
/* Page code (Fixed) */
	0x1B, 
/* Page Length (Fixed) */		
	0x0A,
/* SFLP & SRFP  */	
	0x00,
/* NCD & SML & TLUN */	
	0x01, /* one logical uint */
/* Reserved */	
	0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00
};

/* Timer and Protect Page */
#if defined (__GNUC__)
static UINT8 Mode_Page_1C[8]  __attribute__((aligned(4))) = {
#else
__align(4) static UINT8 Mode_Page_1C[8] = {
#endif
/* Page code (Fixed) */
	0x1C, 
/* Page Length (Fixed) */		
	0x06,
/* Reserved */		
	0x00, 
/* Inactivity Time Multiplier */	
	0x05, /* 2 sec */
/* DISP & SWPP */	
	0x00, 
/* Reserved */	
	0x00, 0x00, 0x00
};

#if defined (__GNUC__)
static UINT8 Mode_Page[24]  __attribute__((aligned(4))) = {
#else
__align(4) static UINT8 Mode_Page[24] = {
#endif
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x1C, 0x0A, 0x80, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 
};

/* MSC Bulk */
void mscdMassBulk(void)
{
	UINT32 HCount, DCount,CBWCount;
#ifdef TEST_SD
	INT32 IsSDInsert0,IsSDInsert1,IsSDInsert2;
#endif		
	if (usbdInfo.USBModeFlag)
	{
		if(mscdInfo.Bulk_First_Flag==0)
		{	
			if(g_mscd_block_mode)
			{
				while(!mscdInfo.dwCBW_flag && usbdInfo.USBModeFlag);
			}
			else
			{
				if(!mscdInfo.dwCBW_flag)
					return;
				if(!usbdInfo.USBModeFlag)
					return;
			}
			
			/* CBW Check */							
			CBWCount = inp32(EPB_DATA_CNT) & 0xFFFF;			
			mscdUSB2SDRAM_Bulk(mscdInfo.Mass_Base_Addr, CBWCount);
			mscdFshBuf2CBW();
			
			if((CBWCount != 31)  || CBW_In.CBWD.dCBWSignature != 0x43425355)
			{		
				if(usbdInfo.USBModeFlag)
				{
					/* Invalid CBW */		
		        	outp32(EPA_RSP_SC, EP_HALT);
	    	    	usbdInfo._usbd_haltep = 1;
	        	    mscdInfo.preventflag = 1;    	
	            	mscdInfo.bulkonlycmd = 0;		            
	            }
	            else
	            	return;
			}
			else
			{	
				/* Valid CBW */						
				mscdInfo.Bulk_First_Flag=1;
				mscdInfo.bulkonlycmd = 1;	
				mscdInfo.dwCBW_flag = 0;
				mscdInfo.dwResidueLen = 0;			
			}					
		}
		else
		{
			if(mscdInfo.preventflag == 1 && usbdInfo._usbd_haltep != 1 && usbdInfo._usbd_haltep != 2)
			{				
        		mscdCSW2FshBuf();
        	}	
		}
	}



    if ((usbdInfo.USBModeFlag)&&(mscdInfo.bulkonlycmd==1))
    {
        HCount = CBW_In.CBWD.dCBWDataTferLh.c32;
#ifdef TEST_CDROM        
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
			DCount = (CBW_In.CBWD.CBWCB_In.f32 >> 8) * mscdInfo.SizePerSector * 4; 	
		else
			DCount = (CBW_In.CBWD.CBWCB_In.f32 >> 8) * mscdInfo.SizePerSector;
#else
		DCount = (CBW_In.CBWD.CBWCB_In.f32 >> 8) * mscdInfo.SizePerSector;        
#endif	        
        if (CBW_In.CBWD.OP_Code == UFI_WRITE_10 || CBW_In.CBWD.OP_Code == UFI_WRITE_12)
        {
         	if( CBW_In.CBWD.OP_Code == 0xAA)
        		DCount = (CBW_In.CBWD.CBWCB_In.f32) * mscdInfo.SizePerSector;	        	       
        
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(u32SdCardDetectEnable & MSC_SD_PORT0)
				{
					if(u32SdUserCardDetect & MSC_SD_PORT0)
					{
						UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserCardDetectPort * 0x10;
						UINT32 u32Pin = 1 << u32Sd0UserCardDetectPin;
											
						if(inp32(u32Port) & u32Pin)		
							IsSDInsert0 = TRUE;
						else
							IsSDInsert0 = FALSE;																
					}				
					else
						sicIoctl(2, (INT32)&IsSDInsert0, 0, 0);
				}
				else
					IsSDInsert0 = TRUE;		        
		        
            	if(IsSDInsert0 == FALSE)
	            {
					/* Invalid CBW */		
		        	outp32(EPA_RSP_SC, EP_HALT);
	    	    	usbdInfo._usbd_haltep = 1;
	        	    mscdInfo.preventflag = 1;    	
	            	mscdInfo.bulkonlycmd = 0;		  
	            	return;
	            }		        
			}		
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT1)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT1)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd1UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd1UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert1 = TRUE;
							else
								IsSDInsert1 = FALSE;																
						}				
						else
							IsSDInsert1 = TRUE;
					}
					else
						IsSDInsert1 = TRUE;		
						
					if(IsSDInsert1 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        
			}
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT2)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT2)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd2UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd2UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert2 = TRUE;
							else
								IsSDInsert2 = FALSE;																
						}				
						else
							IsSDInsert2 = TRUE;
					}
					else
						IsSDInsert2 = TRUE;		
						
					if(IsSDInsert2 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        		        
			}						
#endif        

        	if( CBW_In.CBWD.OP_Code == UFI_WRITE_12)
        		DCount = (CBW_In.CBWD.CBWCB_In.f32) * mscdInfo.SizePerSector;	
        		        
        	if(CBW_In.CBWD.bmCBWFlags == 0x00) /* OUT */
        	{
	        	/* Ho == Do (Case 12) */	
	        	if(HCount == DCount)
	        	{
		            mscdWt10_Command();
		            mscdCSW2FshBuf();
	        	}        
	        	/* Hn < Do (Case 3) || Ho < Do (Case 13) */
	        	else if(HCount < DCount)
	        	{
	        		if(HCount)	/* Ho < Do (Case 13) */
	        			mscdWt10_Command();
	    	    	
	        		mscdInfo.preventflag = 1;
	   	    		mscdCSW2FshBuf();    	    	
	        	} 
	        	/* Ho > Do (Case 11) */
	       		else if(HCount > DCount)
	        	{
	        		mscdInfo.dwResidueLen = DCount;
	        		mscdWt10_Command();
	    	    	mscdInfo.MB_Invalid_Cmd_Flg = 1;
	        		mscdInfo.preventflag = 1;
	   	    		mscdCSW2FshBuf();    	    	
	        	} 
        	}
        	/* Hi <> Do (Case 8) */
        	else
        	{
        		mscdInfo.MB_Invalid_Cmd_Flg = 1;
        		outp32(EPA_RSP_SC, EP_HALT);
    	    	usbdInfo._usbd_haltep = 1;
	            mscdInfo.preventflag = 1;            	

	        }
        }
        else if (CBW_In.CBWD.OP_Code == UFI_READ_10 || CBW_In.CBWD.OP_Code == UFI_READ_12)
  		{
        	if( CBW_In.CBWD.OP_Code == UFI_READ_12)
#ifdef TEST_CDROM
			{
        		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
	        		DCount = (CBW_In.CBWD.CBWCB_In.f32) * mscdInfo.SizePerSector * 4;	
	        	else
	        		DCount = (CBW_In.CBWD.CBWCB_In.f32) * mscdInfo.SizePerSector;
	        }
#else
				DCount = (CBW_In.CBWD.CBWCB_In.f32) * mscdInfo.SizePerSector;	
#endif
                
        
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(u32SdCardDetectEnable & MSC_SD_PORT0)
				{
					if(u32SdUserCardDetect & MSC_SD_PORT0)
					{
						UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserCardDetectPort * 0x10;
						UINT32 u32Pin = 1 << u32Sd0UserCardDetectPin;
											
						if(inp32(u32Port) & u32Pin)		
							IsSDInsert0 = TRUE;
						else
							IsSDInsert0 = FALSE;																
					}				
					else
						sicIoctl(2, (INT32)&IsSDInsert0, 0, 0);
				}
				else
					IsSDInsert0 = TRUE;		        
		        
            	if(IsSDInsert0 == FALSE)
	            {
					/* Invalid CBW */		
		        	outp32(EPA_RSP_SC, EP_HALT);
	    	    	usbdInfo._usbd_haltep = 1;
	        	    mscdInfo.preventflag = 1;    	
	            	mscdInfo.bulkonlycmd = 0;		  
	            	return;
	            }			        
			}	

		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT1)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT1)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd1UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd1UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert1 = TRUE;
							else
								IsSDInsert1 = FALSE;																
						}				
						else
							IsSDInsert1 = TRUE;
					}
					else
						IsSDInsert1 = TRUE;		
						
					if(IsSDInsert1 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        		        
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT2)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT2)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd2UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd2UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert2 = TRUE;
							else
								IsSDInsert2 = FALSE;																
						}				
						else
							IsSDInsert2 = TRUE;
					}
					else
						IsSDInsert2 = TRUE;		
						
					if(IsSDInsert2 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        			        
			}	
#endif

        	if(CBW_In.CBWD.bmCBWFlags == 0x80)	/* IN */
        	{
		       	if(HCount == DCount)	/* Hi == Di (Case 6) */
	        	{
#ifdef TEST_CDROM					
					if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
					{						
						if (bFirstCMD == 0xFF)
							bFirstCMD = 0x5A;							        
										        
				        mscdCDRead(CBW_In.CBWD.LBA.d32, CBW_In.CBWD.dCBWDataTferLh.c32);
					}
					else
						mscdRd10_Command();
#else
	        		mscdRd10_Command();
#endif	        		
	        		mscdCSW2FshBuf();
	        	}
	        	/* Hn < Di (Case 2) || Hi < Di (Case 7) */	
	        	else if(HCount < DCount)
	        	{
	        		if(HCount) /* Hi < Di (Case 7) */
	    	   		{
	        			outp32(EPA_RSP_SC, EP_HALT);
	    	    		usbdInfo._usbd_haltep = 1;
		            	mscdInfo.preventflag = 1;      
	        		}
	        		else	/* Hn < Di (Case 2) */
	        		{
		        		mscdInfo.preventflag = 1;
	        			mscdCSW2FshBuf();    		
	        		}
	        	}
	        	/* Hi > Dn (Case 4) || Hi > Di (Case 5) */
	        	else if(HCount > DCount)
	        	{               
	        		outp32(EPA_RSP_SC, EP_HALT);
	        		usbdInfo._usbd_haltep = 1;
	            	mscdInfo.preventflag = 1;
	            }
	        }
	        else
	        {
	        	/* Ho <> Di (Case 10) */
	        	mscdInfo.MB_Invalid_Cmd_Flg = 1;
	        	outp32(EPB_RSP_SC, EP_HALT);
	    	   	usbdInfo._usbd_haltep = 2;
		    	mscdInfo.preventflag = 1;            	
	        }            
        }
        else if (CBW_In.CBWD.OP_Code == UFI_INQUIRY)
        {
            mscdInquiry_Command();
            mscdCSW2FshBuf();
        }else if (CBW_In.CBWD.OP_Code == UFI_READ_FORMAT_CAPACITY)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(u32SdCardDetectEnable & MSC_SD_PORT0)
				{
					if(u32SdUserCardDetect & MSC_SD_PORT0)
					{
						UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserCardDetectPort * 0x10;
						UINT32 u32Pin = 1 << u32Sd0UserCardDetectPin;
											
						if(inp32(u32Port) & u32Pin)		
							IsSDInsert0 = TRUE;
						else
							IsSDInsert0 = FALSE;																
					}				
					else
						sicIoctl(2, (INT32)&IsSDInsert0, 0, 0);
				}
				else
					IsSDInsert0 = TRUE;		        
		        
            	if(IsSDInsert0 == FALSE)
	            {
					/* Invalid CBW */		
		        	outp32(EPA_RSP_SC, EP_HALT);
	    	    	usbdInfo._usbd_haltep = 1;
	        	    mscdInfo.preventflag = 1;    	
	            	mscdInfo.bulkonlycmd = 0;		  
	            	return;
	            }			        
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT1)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT1)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd1UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd1UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert1 = TRUE;
							else
								IsSDInsert1 = FALSE;																
						}				
						else
							IsSDInsert1 = TRUE;
					}
					else
						IsSDInsert1 = TRUE;		
						
					if(IsSDInsert1 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        		        
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
		        if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT2)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT2)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd2UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd2UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert2 = TRUE;
							else
								IsSDInsert2 = FALSE;																
						}				
						else
							IsSDInsert2 = TRUE;
					}
					else
						IsSDInsert2 = TRUE;		
						
					if(IsSDInsert2 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        	
			}	
#endif        

            mscdRdFmtCap_Command();
            mscdCSW2FshBuf();
        }else if (CBW_In.CBWD.OP_Code == UFI_READ_CAPACITY)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(u32SdCardDetectEnable & MSC_SD_PORT0)
				{
					if(u32SdUserCardDetect & MSC_SD_PORT0)
					{
						UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserCardDetectPort * 0x10;
						UINT32 u32Pin = 1 << u32Sd0UserCardDetectPin;
											
						if(inp32(u32Port) & u32Pin)		
							IsSDInsert0 = TRUE;
						else
							IsSDInsert0 = FALSE;																
					}				
					else
						sicIoctl(2, (INT32)&IsSDInsert0, 0, 0);
				}
				else
					IsSDInsert0 = TRUE;		        
		        
            	if(IsSDInsert0 == FALSE)
	            {
					/* Invalid CBW */		
		        	outp32(EPA_RSP_SC, EP_HALT);
	    	    	usbdInfo._usbd_haltep = 1;
	        	    mscdInfo.preventflag = 1;    	
	            	mscdInfo.bulkonlycmd = 0;		  
	            	return;
	            }			        
			}		
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT1)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT1)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd1UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd1UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert1 = TRUE;
							else
								IsSDInsert1 = FALSE;																
						}				
						else
							IsSDInsert1 = TRUE;
					}
					else
						IsSDInsert1 = TRUE;		
						
					if(IsSDInsert1 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        		        
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
				if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)	
				{
					if(u32SdCardDetectEnable & MSC_SD_PORT2)
					{
						if(u32SdUserCardDetect & MSC_SD_PORT2)
						{
							UINT32 u32Port = REG_GPIOA_PIN + u32Sd2UserCardDetectPort * 0x10;
							UINT32 u32Pin =  1 << u32Sd2UserCardDetectPin;
												
							if(inp32(u32Port) & u32Pin)		
								IsSDInsert2 = TRUE;
							else
								IsSDInsert2 = FALSE;																
						}				
						else
							IsSDInsert2 = TRUE;
					}
					else
						IsSDInsert2 = TRUE;		
						
					if(IsSDInsert2 == FALSE)
		            {
						/* Invalid CBW */		
			        	outp32(EPA_RSP_SC, EP_HALT);
		    	    	usbdInfo._usbd_haltep = 1;
		        	    mscdInfo.preventflag = 1;    	
		            	mscdInfo.bulkonlycmd = 0;		  
		            	return;
		            }		        						
				}        			        
			}	        
#endif        
            mscdRdCurCap_Command();
            mscdCSW2FshBuf();
        }
        else if (CBW_In.CBWD.OP_Code == UFI_MODE_SENSE_10)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}		   
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
#endif

#ifdef TEST_CDROM            
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
			{
				if (bFirstCMD == 0xFF)
					bFirstCMD = 0x5A;	 
	            mscdModeSense_Command_CDROM();
	        }
			else
				mscdModeSense_Command(); 		            
#else
			mscdModeSense_Command();            
#endif            
            mscdCSW2FshBuf();
        }
        else if (CBW_In.CBWD.OP_Code == UFI_MODE_SENSE_6)
        {
            mscdModeSense6_Command();
            mscdCSW2FshBuf();
        }
        else if (CBW_In.CBWD.OP_Code == UFI_TEST_UNIT_READY)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}
#endif        
  
#ifdef TEST_CDROM
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		    { 
				if (bFirstCMD == 0x25)
					bIsDeviceReady = TRUE;   
				
			}

#endif			
        	if(HCount != 0)
        	{
        		if(CBW_In.CBWD.bmCBWFlags == 0x00)	/* Ho > Dn (Case 9) */
        		{
	        		outp32(EPB_RSP_SC, EP_HALT);
	        		usbdInfo._usbd_haltep = 2;
	            	mscdInfo.preventflag = 1;         	
	        	}
        	
        	}
        	else
        	{
				     
#ifdef TEST_CDROM
			    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
			    { 
					// Medium Not Present
					if (!bIsDeviceReady)
					{
						mscdInfo.SenseKey = 0x02;
						mscdInfo.ASC = 0x3a;
						mscdInfo.ASCQ = 0;					
					}	        	
				}
        	
#endif	        	
        	   	mscdCSW2FshBuf();	/* Hn == Dn (Case 1) */
        	}
        
        }else if (CBW_In.CBWD.OP_Code == UFI_PREVENT_ALLOW_MEDIUM_REMOVAL)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}		  
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
#endif        
      
        
            if (CBW_In.CBW_Array[19]==1)
            {
                mscdInfo.preventflag=1;
                
                mscdInfo.SenseKey = 0x05;
                mscdInfo.ASC= 0x24;
                mscdInfo.ASCQ = 0x00;
            }
            else
                mscdInfo.preventflag=0;
            mscdCSW2FshBuf();
        }else if (CBW_In.CBWD.OP_Code == UFI_REQUEST_SENSE)
        {
            mscdReqSen_Command();
            mscdCSW2FshBuf();
        }else if (CBW_In.CBWD.OP_Code == UFI_VERIFY_10)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag0==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}		    
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag1==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        
		    	if (mscdInfo.card_remove_flag2==1)
		        {
					 CSW_In.CSWD.bCSWStatus = 1;        	
		        	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        }
			}	
#endif        
    
        
            mscdCSW2FshBuf();
        }else if (CBW_In.CBWD.OP_Code == UFI_START_STOP)
        {
#ifdef TEST_SD
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		    {        	
		    	if(CBW_In.CBWD.LBA.d32 == 0x200 && mscdInfo.card_remove_flag0 != 1) /* Eject */ 
		    		card_eject_flag0 = 1;		 		
		    	else
		    	{
			    	if (mscdInfo.card_remove_flag0==1)		    	
			        {
						 CSW_In.CSWD.bCSWStatus = 1;        	
		    	    	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        	}
		        }
			}		    
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		    {        	
		    	if(CBW_In.CBWD.LBA.d32 == 0x200 && mscdInfo.card_remove_flag1 != 1) /* Eject */ 
		    		card_eject_flag1 = 1;		 		
		    	else
		    	{
			    	if (mscdInfo.card_remove_flag1==1)		    	
			        {
						 CSW_In.CSWD.bCSWStatus = 1;        	
		    	    	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        	}
		        }
			}	
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		    {        	
		    	if(CBW_In.CBWD.LBA.d32 == 0x200 && mscdInfo.card_remove_flag2 != 1) /* Eject */ 
		    		card_eject_flag2 = 1;		 		
		    	else
		    	{
			    	if (mscdInfo.card_remove_flag2==1)		    	
			        {
						 CSW_In.CSWD.bCSWStatus = 1;        	
		    	    	 CSW_In.CSWD.dCSWDataResidue.g32 = 0;
		        	}
		        }
			}							
#endif        
            mscdCSW2FshBuf();
        }else if (CBW_In.CBWD.OP_Code == UFI_MODE_SELECT_10)
        {
            mscdModeSel_Command();
            mscdCSW2FshBuf();
        }
#ifdef TEST_CDROM
        else if (CBW_In.CBWD.OP_Code == CDROM_COMMAND_43)
        {
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		    { 
	            mscdCommand_43();
    	        mscdCSW2FshBuf();
    	    }
        }
        else if (CBW_In.CBWD.OP_Code == CDROM_COMMAND_46)
        {
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		    {         
				mscdCommand_46();
          		mscdCSW2FshBuf();
    	    }
        }        
        else if (CBW_In.CBWD.OP_Code == CDROM_COMMAND_51)
        {
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		    {         
	            mscdCommand_51();
    	        mscdCSW2FshBuf();
    	    }
        }        
        else if (CBW_In.CBWD.OP_Code == CDROM_COMMAND_4A)
        {
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		    {         
	            mscdCommand_4A();
    	        mscdCSW2FshBuf();
			}    	        
        }        
        else if (CBW_In.CBWD.OP_Code == CDROM_COMMAND_A4)
        {
		    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		    {         
	            mscdCommand_A4();
    	        mscdCSW2FshBuf();
			}    	        
        }            
        
#endif       
        else
        {
            mscdInfo.dwUsedBytes = 0;
            /* INVALID FIELD IN COMMAND PACKET */
            mscdInfo.SenseKey = 0x05;
            mscdInfo.ASC= 0x24;
            mscdInfo.ASCQ = 0x00;
            mscdInfo.MB_Invalid_Cmd_Flg=1;
            mscdCSW2FshBuf(); 
        }
        mscdInfo.bulkonlycmd=0;
    }
}

/* Get CBW from USB buffer */
void mscdFshBuf2CBW(void)
{
	UINT32 i,j;

	for (i = 0 ; i < 17 ; i++)
		CBW_In.CBW_Array[i] = inp8(mscdInfo.Mass_Base_Addr+i);

	j = mscdInfo.Mass_Base_Addr+20;
	for (i = 17 ; i < 21 ; i++)
		CBW_In.CBW_Array[i] = inp8(j--);

	j = mscdInfo.Mass_Base_Addr+24;
	for (i = 21 ; i < 25 ; i++)
		CBW_In.CBW_Array[i] = inp8(j--);

	for (i = 25 ; i < 31 ; i++)
		CBW_In.CBW_Array[i] = inp8(mscdInfo.Mass_Base_Addr+i);
} 

/* Set CSW to USB buffer */
void mscdCSW2FshBuf(void)
{
	UINT8 i;

	for (i = 0 ; i < 13 ; i++)
		CSW_In.CSW_Array[i] = 0x00;

	CSW_In.CSWD.dCSWSignature[0] = 0x55;
	CSW_In.CSWD.dCSWSignature[1] = 0x53;
	CSW_In.CSWD.dCSWSignature[2] = 0x42;
	CSW_In.CSWD.dCSWSignature[3] = 0x53;

	for (i = 0 ; i < 4 ; i++)          
		CSW_In.CSWD.dCSWTag[i] = CBW_In.CBWD.dCBWTag[i];

    outp8(mscdInfo.Mass_Base_Addr+12,0x00);

    if(mscdInfo.MB_Invalid_Cmd_Flg==1)
    {
    	if(mscdInfo.dwResidueLen)
			mscdInfo.dwUsedBytes = CBW_In.CBWD.dCBWDataTferLh.c32- mscdInfo.dwResidueLen;
		else		
	        mscdInfo.dwUsedBytes = CBW_In.CBWD.dCBWDataTferLh.c32 - mscdInfo.dwUsedBytes;

        CSW_In.CSWD.dCSWDataResidue.g[0] = (UINT8)(mscdInfo.dwUsedBytes);
        CSW_In.CSWD.dCSWDataResidue.g[1] = (UINT8)(mscdInfo.dwUsedBytes >> 8 );
        CSW_In.CSWD.dCSWDataResidue.g[2] = (UINT8)(mscdInfo.dwUsedBytes >> 16);
        CSW_In.CSWD.dCSWDataResidue.g[3] = (UINT8)(mscdInfo.dwUsedBytes >> 24);
        outp8(mscdInfo.Mass_Base_Addr+12,0x01);
    }

    CSW_In.CSWD.bCSWStatus = mscdInfo.preventflag;

    for (i = 0 ; i < 13 ; i++)
        outp8(mscdInfo.Mass_Base_Addr+i,CSW_In.CSW_Array[i]);
#ifdef TEST_SD
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
    {
        if ((mscdInfo.card_remove_flag0==1)&&(CBW_In.CBWD.OP_Code == 0x00))
            outp8(mscdInfo.Mass_Base_Addr+12,0x01);
        if (mscdInfo.no_card_flag0==1)
        {
            if((CBW_In.CBWD.OP_Code == 0x12)||(CBW_In.CBWD.OP_Code == 0x03))
                outp8(mscdInfo.Mass_Base_Addr+12,0x00);
            else
                outp8(mscdInfo.Mass_Base_Addr+12,0x01);
        }
    }
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
    {
        if ((mscdInfo.card_remove_flag1==1)&&(CBW_In.CBWD.OP_Code == 0x00))
            outp8(mscdInfo.Mass_Base_Addr+12,0x01);
        if (mscdInfo.no_card_flag1==1)
        {
            if((CBW_In.CBWD.OP_Code == 0x12)||(CBW_In.CBWD.OP_Code == 0x03))
                outp8(mscdInfo.Mass_Base_Addr+12,0x00);
            else
                outp8(mscdInfo.Mass_Base_Addr+12,0x01);
        }
    }
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
    {
        if ((mscdInfo.card_remove_flag2==1)&&(CBW_In.CBWD.OP_Code == 0x00))
            outp8(mscdInfo.Mass_Base_Addr+12,0x01);
        if (mscdInfo.no_card_flag2==1)
        {
            if((CBW_In.CBWD.OP_Code == 0x12)||(CBW_In.CBWD.OP_Code == 0x03))
                outp8(mscdInfo.Mass_Base_Addr+12,0x00);
            else
                outp8(mscdInfo.Mass_Base_Addr+12,0x01);
        }
    }        
#endif
	mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr, 0x0d);
	mscdInfo.Bulk_First_Flag=0;
    mscdInfo.MB_Invalid_Cmd_Flg =0;
    mscdInfo.dwUsedBytes = 0;	
    if((inp32(EPB_DATA_CNT) & 0xFFFF) == 0)
    mscdInfo.dwCBW_flag = 0;	
	
}

/* Inquiry Command */
void mscdInquiry_Command(void)
{
   int i;
    UINT32 wd;
	UINT8 ID[255];
	
	memcpy(ID,InquiryID,sizeof(InquiryID));	
	
#ifdef TEST_RAM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
    {  
    	strcpy((char *)&ID[16],"RAM Disk");
	}		    
#endif  	
	
#ifdef TEST_SD
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
    {        
    	/* Removable Media */
    	if(!((g_MSC_SD_PORT_ENABLE & ~0xF) & MSC_SD_MP_PORT0))
			ID[1] = 0x80;
		strcpy((char *)&ID[16],"SD0 Card Reader");
	}		
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
    {        
    	/* Removable Media */
    	if(!((g_MSC_SD_PORT_ENABLE & ~0xF) & MSC_SD_MP_PORT1))
			ID[1] = 0x80;
		strcpy((char *)&ID[16],"SD1 Card Reader");
	}	
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
    { 
    	/* Removable Media */
    	if(!((g_MSC_SD_PORT_ENABLE & ~0xF) & MSC_SD_MP_PORT2))
			ID[1] = 0x80;
		strcpy((char *)&ID[16],"SD2 Card Reader");
	}			

#endif  	
#ifdef TEST_SPI
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
    {  
	    ID[1] = 0x80;
    	strcpy((char *)&ID[16],"SPI Disk");
	}		    
#endif 

#ifdef TEST_SM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
    {        
		strcpy((char *)&ID[16],"MSC NAND0");
	}		    

    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
    {        
		strcpy((char *)&ID[16],"MSC NAND1");
	}		    	    
#endif  
	
#ifdef TEST_CDROM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
    {        
    	ID[0] = 0x05;
    	ID[1] = 0x80;
    	ID[3] = 0x32;
		strcpy((char *)&ID[16],"MSC CDROM");
	}		
#endif  		
    wd = CBW_In.CBWD.dCBWDataTferLh.c32;
    if(wd >sizeof(ID))	
    	wd = sizeof(ID);
    for (i = 0 ; i < wd; i++)
        outp8(mscdInfo.Mass_Base_Addr+i,ID[i]);
    mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr,wd);
}

/* Read Format Capacity Command */
void mscdRdFmtCap_Command(void)
{
    int i;
    UINT32 rd;
    UINT32 tmpval;
    UINT32 TotalSectors; 
     
    tmpval=mscdInfo.Mass_Base_Addr;
    for (i = 0 ; i < 36 ; i++)
        outp8(tmpval+i,0x00);
    outp8(tmpval+3,0x10);
    
#ifdef TEST_RAM
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
		TotalSectors = mscdInfo.gTotalSectors_RAM;
#endif
#ifdef TEST_SPI
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
		TotalSectors = mscdInfo.gTotalSectors_SPI;
#endif

#ifdef TEST_SD
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		TotalSectors = mscdInfo.gTotalSectors_SD0;
		
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		TotalSectors = mscdInfo.gTotalSectors_SD1;
		
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		TotalSectors = mscdInfo.gTotalSectors_SD2;				
#endif
#ifdef TEST_SM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		TotalSectors = mscdInfo.gTotalSectors_NAND0;

    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		TotalSectors = mscdInfo.gTotalSectors_NAND1;
#endif
    
    outp8(tmpval+4,*((UINT8 *)&TotalSectors+3));
    outp8(tmpval+5,*((UINT8 *)&TotalSectors+2));
    outp8(tmpval+6,*((UINT8 *)&TotalSectors+1));
    outp8(tmpval+7,*((UINT8 *)&TotalSectors+0));
    outp8(tmpval+8,0x02);
    outp8(tmpval+10,0x02);
    outp8(tmpval+12,*((UINT8 *)&TotalSectors+3));
    outp8(tmpval+13,*((UINT8 *)&TotalSectors+2));
    outp8(tmpval+14,*((UINT8 *)&TotalSectors+1));
    outp8(tmpval+15,*((UINT8 *)&TotalSectors+0));
    outp8(tmpval+18,0x02);

    rd = CBW_In.CBWD.dCBWDataTferLh.c32;
    mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr,rd);
}

/* Read Capacity Command */
void mscdRdCurCap_Command(void)
{
	int i;
	UINT32 tmpval,temp;

	tmpval = mscdInfo.Mass_Base_Addr;
	for (i = 0 ; i < 36 ; i++)
		outp8(tmpval+i, 0x00);
	
#ifdef TEST_RAM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
		temp = mscdInfo.gTotalSectors_RAM - 1;
#endif
#ifdef TEST_SPI
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
		temp = mscdInfo.gTotalSectors_SPI - 1;
#endif

#ifdef TEST_SD
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		temp = mscdInfo.gTotalSectors_SD0 - 1;
		
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		temp = mscdInfo.gTotalSectors_SD1 - 1;
		
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		temp = mscdInfo.gTotalSectors_SD2 - 1;				
#endif
#ifdef TEST_SM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		temp = mscdInfo.gTotalSectors_NAND0 - 1;

    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		temp = mscdInfo.gTotalSectors_NAND1 - 1;
#endif 	
#ifdef TEST_CDROM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		temp = mscdInfo.gTotalSectors_CDROM - 1;
#endif		
		
	outp8(tmpval, *((UINT8 *)&temp+3));
	outp8(tmpval+1, *((UINT8 *)&temp+2));
	outp8(tmpval+2, *((UINT8 *)&temp+1));
	outp8(tmpval+3, *((UINT8 *)&temp+0));
	outp8(tmpval+6, 0x02);
#ifdef TEST_CDROM
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_CDROM_LUN)
		outp8(tmpval+6, 0x08);					
#endif		
	
	mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr, CBW_In.CBWD.dCBWDataTferLh.c32);
}
#ifdef TEST_CDROM 
void mscdCDRead(UINT32 lba, UINT32 len)
{
	UINT32 u32Address = 0;
	
	if(pfnMSC_CDROM_CALLBACK != NULL)
		pfnMSC_CDROM_CALLBACK(&u32Address, lba * 2048, len);
		
	mscdSDRAM2USB_Bulk((UINT32)u32Address, len);	
}
#endif
/* Read(10) & Read(12) Command */
void mscdRd10_Command(void)
{
	UINT32 len,lba;
	UINT32 u32Address;
	UINT32 volatile sector_count = 0, loop = 0 , i, sector_offset = 0, sector_start;
	len = CBW_In.CBWD.dCBWDataTferLh.c32;
	sector_count = len /512;	

	sector_start = CBW_In.CBWD.LBA.d32;	
	
	loop = sector_count / MSC_BUFFER_SECTOR;	

	for(i=0;i<loop;i++)
	{	
	#ifdef TEST_RAM	
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
		{
			lba = ((sector_start + sector_offset)*mscdInfo.SizePerSector) + mscdInfo.Storage_Base_Addr_RAMDISK;
		
			u32Address = lba;
		}
	#endif
#ifdef TEST_SPI	
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
	{
		SPI_Read(sector_start + sector_offset, MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);
	
		u32Address = mscdInfo.Storage_Base_Addr;
	}
#endif	

	#ifdef TEST_SD
		
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		{
			if (mscdInfo.card_remove_flag0==0)  
				sicSdRead0(sector_start + sector_offset,MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);
				
			u32Address = mscdInfo.Storage_Base_Addr;
		}
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		{
			if (mscdInfo.card_remove_flag1==0)  
				sicSdRead1(sector_start + sector_offset, MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);	

			u32Address = mscdInfo.Storage_Base_Addr;
		}	
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		{
			lba = CBW_In.CBWD.LBA.d32;	
			if (mscdInfo.card_remove_flag2==0)  
				sicSdRead2(sector_start + sector_offset, MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);
			
			u32Address = mscdInfo.Storage_Base_Addr;
		}	
	#endif

	#ifdef TEST_SM
		
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		{
			lba = CBW_In.CBWD.LBA.d32;	
			GNAND_read(ptMassNDisk, sector_start + sector_offset, MSC_BUFFER_SECTOR, (UINT8 *)mscdInfo.Storage_Base_Addr);
		
			u32Address = mscdInfo.Storage_Base_Addr;	
		}
		
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		{
			lba = CBW_In.CBWD.LBA.d32;	
			GNAND_read(ptMassNDisk1, sector_start + sector_offset, MSC_BUFFER_SECTOR, (UINT8 *)mscdInfo.Storage_Base_Addr);
		
			u32Address = mscdInfo.Storage_Base_Addr;	
		}
	#endif

		mscdSDRAM2USB_Bulk(u32Address, MSC_BUFFER_SECTOR * 512);	
		sector_offset += MSC_BUFFER_SECTOR;
		sector_count -= MSC_BUFFER_SECTOR;	
	}
	if(sector_count % MSC_BUFFER_SECTOR)	
	{
	#ifdef TEST_RAM	
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
		{
			lba = ((sector_start + sector_offset)*mscdInfo.SizePerSector) + mscdInfo.Storage_Base_Addr_RAMDISK;
		
			u32Address = lba;
		}
	#endif
	#ifdef TEST_SPI
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
		{
			SPI_Read(sector_start + sector_offset, sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);
	
			u32Address = mscdInfo.Storage_Base_Addr;		
		}
	#endif		

	#ifdef TEST_SD
		
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		{
			if (mscdInfo.card_remove_flag0==0)  
				sicSdRead0(sector_start + sector_offset,sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);
				
			u32Address = mscdInfo.Storage_Base_Addr;
		}
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		{
			if (mscdInfo.card_remove_flag1==0)  
				sicSdRead1(sector_start + sector_offset, sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);	

			u32Address = mscdInfo.Storage_Base_Addr;
		}	
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		{
			lba = CBW_In.CBWD.LBA.d32;	
			if (mscdInfo.card_remove_flag2==0)  
				sicSdRead2(sector_start + sector_offset, sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);
			
			u32Address = mscdInfo.Storage_Base_Addr;
		}	
	#endif

	#ifdef TEST_SM
		
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		{
			lba = CBW_In.CBWD.LBA.d32;	
			GNAND_read(ptMassNDisk, sector_start + sector_offset, sector_count, (UINT8 *)mscdInfo.Storage_Base_Addr);
		
			u32Address = mscdInfo.Storage_Base_Addr;	
		}
		
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		{
			lba = CBW_In.CBWD.LBA.d32;	
			GNAND_read(ptMassNDisk1, sector_start + sector_offset, sector_count, (UINT8 *)mscdInfo.Storage_Base_Addr);
		
			u32Address = mscdInfo.Storage_Base_Addr;	
		}

	#endif

		mscdSDRAM2USB_Bulk(u32Address, sector_count * 512);		
	
	}	
}

/* Write(10) & Write(12) */
void mscdWt10_Command(void)
{
	UINT32 len,lba;
	UINT32 volatile sector_count = 0, loop = 0 , i, sector_offset = 0, sector_start;

	len = CBW_In.CBWD.dCBWDataTferLh.c32;	

#ifdef TEST_RAM
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
		lba = (CBW_In.CBWD.LBA.d32*mscdInfo.SizePerSector) + mscdInfo.Storage_Base_Addr_RAMDISK;	
#endif	
#ifdef TEST_SPI
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
		lba = 	mscdInfo.Storage_Base_Addr;	
#endif	
	
#ifdef TEST_SM
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		lba = 	mscdInfo.Storage_Base_Addr;	

	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		lba = 	mscdInfo.Storage_Base_Addr;	
#endif
#ifdef TEST_SD
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		lba = 	mscdInfo.Storage_Base_Addr;	
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		lba = 	mscdInfo.Storage_Base_Addr;	
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		lba = 	mscdInfo.Storage_Base_Addr;					
#endif
	
	
	sector_count = len / 512;
	
	sector_start = CBW_In.CBWD.LBA.d32;	
	
	loop = sector_count / MSC_BUFFER_SECTOR;		
	for(i=0;i<loop;i++)
	{			
	
		mscdUSB2SDRAM_Bulk(lba, MSC_BUFFER_SECTOR * 512);	
		
	#ifdef TEST_SPI
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
		{
			SPI_Write(sector_start + sector_offset, MSC_BUFFER_SECTOR, (UINT8 *)mscdInfo.Storage_Base_Addr);
		}
	#endif
		
	#ifdef TEST_SM
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		{
			GNAND_write(ptMassNDisk,  sector_start + sector_offset, MSC_BUFFER_SECTOR, (UINT8 *)mscdInfo.Storage_Base_Addr);
		}

		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		{
			GNAND_write(ptMassNDisk1, sector_start + sector_offset, MSC_BUFFER_SECTOR, (UINT8 *)mscdInfo.Storage_Base_Addr);
		}

	#endif


	#ifdef TEST_SD
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		{
			if (mscdInfo.card_remove_flag0==0)  
			{
				sicSdWrite0( sector_start + sector_offset, MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);			
			}
		}
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		{
			if (mscdInfo.card_remove_flag1==0)  
			{
				sicSdWrite1( sector_start + sector_offset, MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);
			}
		}	
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		{
			if (mscdInfo.card_remove_flag2==0)  
			{
				sicSdWrite2( sector_start + sector_offset, MSC_BUFFER_SECTOR, (unsigned int)mscdInfo.Storage_Base_Addr);
			}		
		}	
	#endif
		sector_offset += MSC_BUFFER_SECTOR;
		sector_count -= MSC_BUFFER_SECTOR;
	}	
	if(sector_count % MSC_BUFFER_SECTOR)	
	{	
		mscdUSB2SDRAM_Bulk(lba, sector_count * 512);	
		
	#ifdef TEST_SPI
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
		{
			SPI_Write(sector_start + sector_offset, sector_count, (UINT8 *)mscdInfo.Storage_Base_Addr);
		}
	#endif
		
	#ifdef TEST_SM
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
		{
			GNAND_write(ptMassNDisk,  sector_start + sector_offset, sector_count, (UINT8 *)mscdInfo.Storage_Base_Addr);
		}

		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
		{
			GNAND_write(ptMassNDisk1, sector_start + sector_offset, sector_count, (UINT8 *)mscdInfo.Storage_Base_Addr);
		}

	#endif


	#ifdef TEST_SD
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
		{
			if (mscdInfo.card_remove_flag0==0)  
			{
				sicSdWrite0( sector_start + sector_offset, sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);			
			}
		}
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
		{
			if (mscdInfo.card_remove_flag1==0)  
			{
				sicSdWrite1( sector_start + sector_offset, sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);
			}
		}	
		if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
		{
			if (mscdInfo.card_remove_flag2==0)  
			{
				sicSdWrite2( sector_start + sector_offset, sector_count, (unsigned int)mscdInfo.Storage_Base_Addr);
			}		
		}	
	#endif
	}	
		
}


void mscdModeSense6_Command(void)
{
	UINT8 i;

	for (i = 0; i<4; i++)
		outp8(mscdInfo.Mass_Base_Addr+i,Mode_Page[i]);

	mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr,CBW_In.CBWD.dCBWDataTferLh.c32);
}

/* Request Sense Command */
void mscdReqSen_Command(void)
{
    UINT8 i;

    for (i = 0 ; i < 18 ; i++)
        outp8(mscdInfo.Mass_Base_Addr+i,0x00);
    if (mscdInfo.preventflag==1)
    {
        mscdInfo.preventflag=0;
        outp8(mscdInfo.Mass_Base_Addr,0x70);
    }
    else
        outp8(mscdInfo.Mass_Base_Addr,0xf0);

#ifdef TEST_SD
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
    { 	
        if (mscdInfo.no_card_flag0 == 1 || mscdInfo.card_remove_flag0 == 1)
        {
        	/* MEDIUM NOT PRESENT */
            mscdInfo.SenseKey = 0x02;
            mscdInfo.ASC= 0x3a;
            mscdInfo.ASCQ = 0x00;
        }
		        
        if ( card_insert_flag0 == 1)
        {
        	/* NOT READY TO READY TRANSITION - MEDIA CHANGED */
            mscdInfo.SenseKey = 0x06;
            card_insert_flag0 = 0;
            mscdInfo.ASC = 0x28;
            mscdInfo.ASCQ = 0x00;
            mscdInfo.card_remove_flag0=0;
        }  
    }
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
    { 	

        if (mscdInfo.no_card_flag1 == 1 || mscdInfo.card_remove_flag1 == 1)
        {
        	/* MEDIUM NOT PRESENT */
            mscdInfo.SenseKey = 0x02;
            mscdInfo.ASC= 0x3a;
            mscdInfo.ASCQ = 0x00;
        }
		        
        if ( card_insert_flag1 == 1)
        {
        	/* NOT READY TO READY TRANSITION - MEDIA CHANGED */
            mscdInfo.SenseKey = 0x06;
            card_insert_flag1 = 0;
            mscdInfo.ASC = 0x28;
            mscdInfo.ASCQ = 0x00;
            mscdInfo.card_remove_flag1=0;
        }  
    }
    if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
    { 	

        if (mscdInfo.no_card_flag2 == 1 || mscdInfo.card_remove_flag2 == 1)
        {
        	/* MEDIUM NOT PRESENT */
            mscdInfo.SenseKey = 0x02;
            mscdInfo.ASC= 0x3a;
            mscdInfo.ASCQ = 0x00;
        }
		        
        if ( card_insert_flag2 == 1)
        {
        	/* NOT READY TO READY TRANSITION - MEDIA CHANGED */
            mscdInfo.SenseKey = 0x06;
            card_insert_flag2 = 0;
            mscdInfo.ASC = 0x28;
            mscdInfo.ASCQ = 0x00;
            mscdInfo.card_remove_flag2=0;
        }  
    }        
#endif

    outp8(mscdInfo.Mass_Base_Addr+2,mscdInfo.SenseKey);
    outp8(mscdInfo.Mass_Base_Addr+7,0x0a);
    outp8(mscdInfo.Mass_Base_Addr+12,mscdInfo.ASC);
    outp8(mscdInfo.Mass_Base_Addr+13,mscdInfo.ASCQ);
    mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr,CBW_In.CBWD.dCBWDataTferLh.c32);
   
    /* NO SENSE */
    mscdInfo.SenseKey = 0;
    mscdInfo.ASC= 0;
    mscdInfo.ASCQ = 0;
}

/* Mode Select Command */
void mscdModeSel_Command(void)
{
    mscdUSB2SDRAM_Bulk(mscdInfo.Mass_Base_Addr,CBW_In.CBWD.dCBWDataTferLh.c32);
}

/* Mode Sense Command */
void mscdModeSense_Command(void)
{	
	UINT8 i,j;

	for (i = 0 ; i < 8 ; i++)
		outp8(mscdInfo.Mass_Base_Addr+i, 0x00);

#ifdef TEST_SD	
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)	
	{
		if(u32SdWriteProtectEnable & MSC_SD_PORT0)
		{
			UINT32 u32Port = REG_GPIOA_PIN + MSC_SD_GPIO_PORTA * 0x10;
			UINT32 u32Pin = BIT0;
			
			if(u32SdUserWriteProtect & MSC_SD_PORT0)
			{
				u32Port = REG_GPIOA_PIN + u32Sd0UserWriteProtectPort * 0x10;
				u32Pin = 1 << u32Sd0UserWriteProtectPin;
			}
			
			if(inp32(u32Port) & u32Pin)		
			{			
				outp8(mscdInfo.Mass_Base_Addr+3,0x80);
				if(g_ShowWriteProtectFlag0)
					sysprintf("SD0 Write-Protection is On\n");				
			}
			else
			{
				if(g_ShowWriteProtectFlag0)
					sysprintf("SD0 Write-Protection is Off\n");	
			}
			g_ShowWriteProtectFlag0 = 0;	
		}
		else
		{
			if(g_ShowWriteProtectFlag0)
				sysprintf("SD0 Write-Protection is Off\n");	
			g_ShowWriteProtectFlag0 = 0;			
		}
	}
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)	
	{
		if(u32SdWriteProtectEnable & MSC_SD_PORT1)
		{
			if(u32SdUserWriteProtect & MSC_SD_PORT1)
			{
				UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserWriteProtectPort * 0x10;
				UINT32 u32Pin = 1 << u32Sd0UserWriteProtectPin;
				if(inp32(u32Port) & u32Pin)		
				{			
					outp8(mscdInfo.Mass_Base_Addr+3,0x80);
					if(g_ShowWriteProtectFlag1)
						sysprintf("SD1 Write-Protection is On\n");				
				}
				else
				{
					if(g_ShowWriteProtectFlag1)
						sysprintf("SD1 Write-Protection is Off\n");	
				}
				g_ShowWriteProtectFlag1 = 0;				
			}
			else
			{
				if(g_ShowWriteProtectFlag1)
					sysprintf("SD1 Write-Protection is Off\n");	
				g_ShowWriteProtectFlag1 = 0;				
			}
		}
		else
		{
			if(g_ShowWriteProtectFlag1)
				sysprintf("SD1 Write-Protection is Off\n");	
			g_ShowWriteProtectFlag1 = 0;			
		}					
	}	
	
	if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)	
	{
		if(u32SdWriteProtectEnable & MSC_SD_PORT2)
		{			
			if(u32SdUserWriteProtect & MSC_SD_PORT2)
			{
				UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserWriteProtectPort * 0x10;
				UINT32 u32Pin = 1 << u32Sd0UserWriteProtectPin;
				if(inp32(u32Port) & u32Pin)		
				{			
					outp8(mscdInfo.Mass_Base_Addr+3,0x80);
					if(g_ShowWriteProtectFlag2)
						sysprintf("SD2 Write-Protection is On\n");				
				}
				else
				{
					if(g_ShowWriteProtectFlag2)
						sysprintf("SD2 Write-Protection is Off\n");	
				}
				g_ShowWriteProtectFlag2 = 0;				
			}
			else
			{
				if(g_ShowWriteProtectFlag2)
					sysprintf("SD2 Write-Protection is Off\n");	
				g_ShowWriteProtectFlag2 = 0;				
			}
		}
		else
		{
			if(g_ShowWriteProtectFlag2)
				sysprintf("SD2 Write-Protection is Off\n");	
			g_ShowWriteProtectFlag2 = 0;			
		}									
	}	
#endif

	switch (CBW_In.CBWD.LBA.d[0])
	{
		case 0x01:
			outp8(mscdInfo.Mass_Base_Addr,19);
			i = 8;
			for (j = 0; j<12; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i,Mode_Page_01[j]);
				i++;
			}
			break;

		case 0x05:
			outp8(mscdInfo.Mass_Base_Addr, 39);
			i = 8;
			for (j = 0; j<32; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i, Mode_Page_05[j]);
				i++;
			}
			mscdInfo.DDB.NumHead = 2;
			mscdInfo.DDB.NumSector = 64;
	
#ifdef TEST_RAM
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_RAM/128;
#endif
#ifdef TEST_SPI
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SPI/128;
#endif
						
#ifdef TEST_SD	
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SD0/128;
				
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SD1/128;
				
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SD2/128;				
#endif

#ifdef TEST_SM
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_NAND0/128;

			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_NAND1/128;
#endif
	
			outp8(mscdInfo.Mass_Base_Addr+12, mscdInfo.DDB.NumHead);
			outp8(mscdInfo.Mass_Base_Addr+13, mscdInfo.DDB.NumSector);
			outp8(mscdInfo.Mass_Base_Addr+16, (UINT8)(mscdInfo.DDB.NumCyl >> 8));
			outp8(mscdInfo.Mass_Base_Addr+17, (UINT8)(mscdInfo.DDB.NumCyl & 0x00ff));
			
			break;

		case 0x1B:
			outp8(mscdInfo.Mass_Base_Addr,19);
			i = 8;
			for (j = 0; j<12; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i,Mode_Page_1B[j]);
				i++;
			}
			break;

		case 0x1C:
			outp8(mscdInfo.Mass_Base_Addr,15);
			i = 8;
			for (j = 0; j<8; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i,Mode_Page_1C[j]);
				i++;
			}
			break;

		case 0x3F:
			outp8(mscdInfo.Mass_Base_Addr, 0x47);
			i = 8;
			for (j = 0; j<12; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i, Mode_Page_01[j]);
				i++;
			}

			for (j = 0; j<32; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i, Mode_Page_05[j]);
				i++;
			}

			for (j = 0; j<12; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i, Mode_Page_1B[j]);
				i++;
			}

			for (j = 0; j<8; j++)
			{
				outp8(mscdInfo.Mass_Base_Addr+i, Mode_Page_1C[j]);
				i++;
			}
			mscdInfo.DDB.NumHead = 2;
			mscdInfo.DDB.NumSector = 64;
						
	
#ifdef TEST_RAM
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_RAM_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_RAM/128;
#endif
#ifdef TEST_SPI
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SPI_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SPI/128;
#endif
						
#ifdef TEST_SD	
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD0_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SD0/128;
				
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD1_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SD1/128;
				
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_SD2_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_SD2/128;				

#endif

#ifdef TEST_SM
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND0_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_NAND0/128;
				
			if (CBW_In.CBWD.bCBWLUN==mscdInfo.F_NAND1_LUN)
				mscdInfo.DDB.NumCyl = mscdInfo.gTotalSectors_NAND1/128;
#endif
			outp8(mscdInfo.Mass_Base_Addr+24, mscdInfo.DDB.NumHead);
			outp8(mscdInfo.Mass_Base_Addr+25, mscdInfo.DDB.NumSector);
			outp8(mscdInfo.Mass_Base_Addr+28, (UINT8)(mscdInfo.DDB.NumCyl >> 8));
			outp8(mscdInfo.Mass_Base_Addr+29, (UINT8)(mscdInfo.DDB.NumCyl & 0x00ff));
			break;

		default:
			 /* INVALID FIELD IN COMMAND PACKET */
			mscdInfo.SenseKey = 0x05;
			mscdInfo.ASC = 0x24;
			mscdInfo.ASCQ = 0;
	}
	mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr, CBW_In.CBWD.dCBWDataTferLh.c32);
}

/* USB Reset Callback function */
VOID MSC_Reset(void)
{
	mscdInfo.Bulk_First_Flag=0;
	mscdInfo._usbd_Less_MPS=0;
}

/* USB Endpoint B Interrupt Callback function */
VOID MSC_EPB_CallBack(UINT32 u32IntEn,UINT32 u32IntStatus)
{
	/* Receive data from HOST (CBW/Data) */
	if(u32IntStatus & DATA_RxED_IS)
		mscdInfo.dwCBW_flag = 1;
}

/* USB Endpoint A Interrupt Callback function */
VOID MSC_EPA_CallBack(UINT32 u32IntEn,UINT32 u32IntStatus)
{
	/* Send data to HOST (CBW/Data) */
	if(u32IntStatus & DATA_TxED_IS)
		mscdInfo.TxedFlag = 1;
}
VOID MSC_DMA_Completion(void)
{

}

/* USB Transfer (DMA configuration) */
void mscdSDRAM_USB_Transfer(UINT32 DRAM_Addr ,UINT32 Tran_Size)
{
	
	if(Tran_Size != 0)
	{		
		outp32(USB_IRQ_ENB, (USB_DMA_REQ | USB_RST_STS | USB_SUS_REQ|VBUS_IE));
		outp32(AHB_DMA_ADDR, DRAM_Addr);
		outp32(DMA_CNT, Tran_Size);
		usbdInfo._usbd_DMA_Flag=0;
		if((inp32(DMA_CTRL_STS) & 0x03) == 0x01)
			mscdInfo.TxedFlag = 0;
		outp32(DMA_CTRL_STS, inp32(DMA_CTRL_STS)|0x00000020);
		
		while(usbdInfo.USBModeFlag)
			if((inp32(DMA_CTRL_STS) & 0x00000020) == 0)
				break;
		

		if((inp32(DMA_CTRL_STS) & 0x03) == 0x01)
		{
			if(Tran_Size % inp32(EPA_MPS) != 0)
				outp32(EPA_RSP_SC, (inp32(EPA_RSP_SC)&0xf7)|0x00000040);	// packet end
			while((usbdInfo.USBModeFlag)&&((inp32(EPA_DATA_CNT) & 0xFFFF) != 0));		
		}
		
	}
}

/* USB means usb host, sdram->host=> bulk in */
void mscdSDRAM2USB_Bulk(UINT32 DRAM_Addr ,UINT32 Tran_Size)
{
	UINT32 volatile count=0;

	mscdInfo._usbd_DMA_Dir = Ep_In;
	
   	outp32(DMA_CTRL_STS, 0x11);	// bulk in, dma read, ep1
	if (Tran_Size < usbdInfo.usbdMaxPacketSize)
	{
		
		mscdInfo._usbd_Less_MPS = 1;
		while(usbdInfo.USBModeFlag)
		{
			if (inp32(EPA_IRQ_STAT) & 0x02)
			{
	        	mscdSDRAM_USB_Transfer(DRAM_Addr, Tran_Size);
	        	break;
	        }
        }
	}
	else if (Tran_Size <= mscdInfo.USBD_DMA_LEN)
	{
		count = Tran_Size / usbdInfo.usbdMaxPacketSize;
		if (count != 0)
		{
			//outp32(EPA_IRQ_ENB, 0x08);
			mscdInfo._usbd_Less_MPS = 0;
			while(usbdInfo.USBModeFlag)
			{
				if (inp32(EPA_IRQ_STAT) & 0x02)
				{
		        	mscdSDRAM_USB_Transfer(DRAM_Addr, usbdInfo.usbdMaxPacketSize*count);
		        	break;
		        }
	        }
        }

		if ((Tran_Size % usbdInfo.usbdMaxPacketSize) != 0)
		{
			//outp32(EPA_IRQ_ENB, 0x08);
			mscdInfo._usbd_Less_MPS = 1;
			while(usbdInfo.USBModeFlag)
			{
				if (inp32(EPA_IRQ_STAT) & 0x02)
				{
			        mscdSDRAM_USB_Transfer((DRAM_Addr+count*usbdInfo.usbdMaxPacketSize),(Tran_Size%usbdInfo.usbdMaxPacketSize));
		        	break;
		        }
	        }
		}
	}
	
}
/* USB means usb host, host->sdram => bulk out */
void mscdUSB2SDRAM_Bulk(UINT32 DRAM_Addr ,UINT32 Tran_Size)
{
	unsigned int volatile count=0;
	int volatile i;

	mscdInfo._usbd_DMA_Dir = Ep_Out;
   	outp32(DMA_CTRL_STS, 0x02);	// bulk out, dma write, ep2

	if (Tran_Size >= mscdInfo.USBD_DMA_LEN)
	{
		count = Tran_Size / mscdInfo.USBD_DMA_LEN;
		for (i=0; i<count; i++)
	        mscdSDRAM_USB_Transfer((DRAM_Addr+i*mscdInfo.USBD_DMA_LEN),mscdInfo.USBD_DMA_LEN);

		if ((Tran_Size % mscdInfo.USBD_DMA_LEN) != 0)
		        mscdSDRAM_USB_Transfer((DRAM_Addr+i*mscdInfo.USBD_DMA_LEN),(Tran_Size%mscdInfo.USBD_DMA_LEN));
	}
	else
        mscdSDRAM_USB_Transfer(DRAM_Addr,Tran_Size);
}

/* USB Class Data IN Callback function for Get MaxLun Command */
VOID MSC_ClassDataIn(void)
{
	if (_usb_cmd_pkt.bRequest == GET_MAX_LUN)
	{		
		if(_usb_cmd_pkt.wValue != 0 || _usb_cmd_pkt.wIndex != 0  || _usb_cmd_pkt.wLength != 1)
		{
			/* Invalid Get MaxLun Command */
			outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));	
			outp32(CEP_CTRL_STAT, CEP_SEND_STALL);			
		}
		else
		{
			/* Valid Get MaxLun Command */
			outp8(CEP_DATA_BUF, (mscdInfo.Mass_LUN - 1));
			outp32(IN_TRNSFR_CNT, 1);			
		}
	}
	else
	{
		/* Valid GET Command */			
		outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));	
		outp32(CEP_CTRL_STAT, CEP_SEND_STALL);			
	}
}

/* USB Class Data OUT Callback function for BOT MSC Reset Request */
VOID MSC_ClassDataOut(void)
{
	if(_usb_cmd_pkt.bRequest == BULK_ONLY_MASS_STORAGE_RESET)
	{
		if(_usb_cmd_pkt.wValue != 0 || _usb_cmd_pkt.wIndex != 0 || _usb_cmd_pkt.wLength != 0)
		{
			/* Invalid BOT MSC Reset Command */
			outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));	
			outp32(CEP_CTRL_STAT, CEP_SEND_STALL);			
			sysprintf("Wrong MSC Reset\n");
		}		
		else
		{
			/* Valid BOT MSC Reset Command */		
			mscdInfo.preventflag = 0;  
			mscdInfo.dwCBW_flag = 0;	
			outp32(CEP_CTRL_STAT, CEP_ZEROLEN);				
			outp32(USB_IRQ_ENB, (RST_IE|SUS_IE|RUM_IE|VBUS_IE));			
			outp32(CEP_IRQ_STAT, ~(CEP_SETUP_TK_IS | CEP_SETUP_PK_IS));		
			outp32(EPA_RSP_SC, 0);
			outp32(EPA_RSP_SC, BUF_FLUSH);	/* flush fifo */
			outp32(EPA_RSP_SC, TOGGLE);
			outp32(EPB_RSP_SC, 0);
			outp32(EPB_RSP_SC, BUF_FLUSH);	/* flush fifo */
			outp32(EPB_RSP_SC, TOGGLE);							
			outp32(CEP_CTRL_STAT,FLUSH);
			sysprintf("MSC Reset\n");
		}
	}
	else
	{
		/* Invalid SET Command */	
		outp32(CEP_IRQ_ENB, (inp32(CEP_IRQ_ENB) | (CEP_SETUP_TK_IE | CEP_SETUP_PK_IE)));	
		outp32(CEP_CTRL_STAT, CEP_SEND_STALL);		
	}
}

/* MSC High Speed Init */
void mscdHighSpeedInit()
{
	usbdInfo.usbdMaxPacketSize = 0x200;
		outp32(EPA_MPS, 0x00000200);		// mps 512
	while(inp32(EPA_MPS) != 0x00000200);		// mps 512
	
	/* bulk in */
	outp32(EPA_IRQ_ENB, 0x00000008);	// tx transmitted
	outp32(EPA_RSP_SC, 0x00000000);		// auto validation
	outp32(EPA_MPS, 0x00000200);		// mps 512
	outp32(EPA_CFG, 0x0000001b);		// bulk in ep no 1
	outp32(EPA_START_ADDR, 0x00000200);
	outp32(EPA_END_ADDR, 0x000003ff);
	
	/* bulk out */
	outp32(EPB_IRQ_ENB, 0x00000010);	// data pkt received  and outtokenb
	outp32(EPB_RSP_SC, 0x00000000);		// auto validation
	outp32(EPB_MPS, 0x00000200);		// mps 512
	outp32(EPB_CFG, 0x00000023);		// bulk out ep no 2
	outp32(EPB_START_ADDR, 0x00000400);
	outp32(EPB_END_ADDR, 0x000005FF);
}

/* MSC Full Speed Init */
VOID mscdFullSpeedInit(void)
{
	usbdInfo.usbdMaxPacketSize = 0x40;
	/* bulk in */
	outp32(EPA_IRQ_ENB, 0x00000008);	// tx transmitted	
	outp32(EPA_RSP_SC, 0x00000000);		// auto validation
	outp32(EPA_MPS, 0x00000040);		// mps 64
	outp32(EPA_CFG, 0x0000001b);		// bulk in ep no 1
	outp32(EPA_START_ADDR, 0x00000100);
	outp32(EPA_END_ADDR, 0x0000017f);

	/* bulk out */
	outp32(EPB_IRQ_ENB, 0x00000010);	// data pkt received  and outtokenb
	outp32(EPB_RSP_SC, 0x00000000);		// auto validation
	outp32(EPB_MPS, 0x00000040);		// mps 64
	outp32(EPB_CFG, 0x00000023);		// bulk out ep no 2
	outp32(EPB_START_ADDR, 0x00000200);
	outp32(EPB_END_ADDR, 0x0000027f);
}
			
/* MSC Event Control */
VOID mscdMassEvent(PFN_USBD_EXIT_CALLBACK* callback_func)
{
#ifdef TEST_SD
	INT32 IsSDInsert0,IsSDInsert1,IsSDInsert2;
#endif		
	if(g_mscd_block_mode && callback_func == NULL)
		return;
	
	do
	{
		if (usbdInfo.USBModeFlag)
			mscdMassBulk();
#ifdef TEST_SD
		if(g_MSC_SD_PORT_ENABLE != MSC_SD_DISABLE)		
		{	
			/* check card status */
			if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
			{
				if(u32SdCardDetectEnable & MSC_SD_PORT0)
				{
					if(u32SdUserCardDetect & MSC_SD_PORT0)
					{
						UINT32 u32Port = REG_GPIOA_PIN + u32Sd0UserCardDetectPort * 0x10;
						UINT32 u32Pin = 1 << u32Sd0UserCardDetectPin;
											
						if(inp32(u32Port) & u32Pin)		
							IsSDInsert0 = TRUE;
						else
							IsSDInsert0 = FALSE;																
					}				
					else
						sicIoctl(2, (INT32)&IsSDInsert0, 0, 0);
				}
				else
					IsSDInsert0 = TRUE;
				
				if(card_eject_flag0 ==1)
				{			
					g_ShowWriteProtectFlag0 = 0;
					sicSdClose0();					
					card_eject_flag0 =2;
					mscdInfo.card_remove_flag0 = 1;
					mscdInfo.no_card_flag0 = 1; 
				}
						
				if (IsSDInsert0 && (mscdInfo.card_remove_flag0 == 1) && (card_eject_flag0 != 2))
				{						
					g_ShowWriteProtectFlag0 = 1;
					card_insert_flag0 = 1;
					Flash_Identify(mscdInfo.F_SD0_LUN);
		            mscdInfo.card_remove_flag0 = 0;  
		            mscdInfo.no_card_flag0 = 0;  
		            card_eject_flag0 = 0;
				}		
				if (!IsSDInsert0 && (mscdInfo.card_remove_flag0 == 0 || card_eject_flag0 == 2)) 
				{
					card_insert_flag0 = 0;
					g_ShowWriteProtectFlag0 = 0;
					sicSdClose0();
		            mscdInfo.card_remove_flag0 = 1;  
		            mscdInfo.no_card_flag0 = 1; 
		            card_eject_flag0 = 0;
				}	
			}
			if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)	
			{
				if(u32SdCardDetectEnable & MSC_SD_PORT1)
				{
					if(u32SdUserCardDetect & MSC_SD_PORT1)
					{
						UINT32 u32Port = REG_GPIOA_PIN + u32Sd1UserCardDetectPort * 0x10;
						UINT32 u32Pin =  1 << u32Sd1UserCardDetectPin;
											
						if(inp32(u32Port) & u32Pin)		
							IsSDInsert1 = TRUE;
						else
							IsSDInsert1 = FALSE;																
					}				
					else
						IsSDInsert1 = TRUE;
				}
				else
					IsSDInsert1 = TRUE;
				
				if(card_eject_flag1 ==1)
				{			
					g_ShowWriteProtectFlag1 = 0;
					sicSdClose1();					
					card_eject_flag1 =2;
					mscdInfo.card_remove_flag1 = 1;
					mscdInfo.no_card_flag1 = 1; 
				}
						
				if (IsSDInsert1 && (mscdInfo.card_remove_flag1 == 1) && (card_eject_flag1 != 2))
				{						
					g_ShowWriteProtectFlag1 = 1;
					card_insert_flag1 = 1;
					Flash_Identify(mscdInfo.F_SD1_LUN);
		            mscdInfo.card_remove_flag1 = 0;  
		            mscdInfo.no_card_flag1 = 0;  
		            card_eject_flag1 = 0;
				}		
				if (!IsSDInsert1 && (mscdInfo.card_remove_flag1 == 0 || card_eject_flag1 == 2)) 
				{
					card_insert_flag1 = 0;
					g_ShowWriteProtectFlag1 = 0;
					sicSdClose1();
		            mscdInfo.card_remove_flag1 = 1;  
		            mscdInfo.no_card_flag1 = 1; 
		            card_eject_flag1 = 0;
				}	
			}
				
			if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)			
			{				
				if(u32SdCardDetectEnable & MSC_SD_PORT2)
				{
					UINT32 u32Port = REG_GPIOA_PIN + MSC_SD_GPIO_PORTE * 0x10;
					UINT32 u32Pin = BIT11;
								
					if(u32SdUserCardDetect & MSC_SD_PORT2)
					{						
						u32Port = REG_GPIOA_PIN + u32Sd2UserCardDetectPort * 0x10;
						u32Pin = 1 << u32Sd2UserCardDetectPin;
					}	
					
					if(inp32(u32Port) & u32Pin)		
						IsSDInsert2 = TRUE;
					else
						IsSDInsert2 = FALSE;	
				}
				else
					IsSDInsert2 = TRUE;						
								
				if(card_eject_flag2 ==1)
				{			
					g_ShowWriteProtectFlag2 = 0;
					sicSdClose2();					
					card_eject_flag2 =2;
					mscdInfo.card_remove_flag2 = 1;
					mscdInfo.no_card_flag2 = 1; 
				}
						
				if (IsSDInsert2 && (mscdInfo.card_remove_flag2 == 1) && (card_eject_flag2 != 2))
				{						
					g_ShowWriteProtectFlag2 = 1;
					card_insert_flag2 = 1;
					Flash_Identify(mscdInfo.F_SD2_LUN);
		            mscdInfo.card_remove_flag2 = 0;  
		            mscdInfo.no_card_flag2 = 0;  
		            card_eject_flag2 = 0;
				}		
				if (!IsSDInsert2 && (mscdInfo.card_remove_flag2 == 0 || card_eject_flag2 == 2)) 
				{
					card_insert_flag2 = 0;
					g_ShowWriteProtectFlag2 = 0;
					sicSdClose2();
		            mscdInfo.card_remove_flag2 = 1;  
		            mscdInfo.no_card_flag2 = 1; 
		            card_eject_flag2 = 0;
				}	
			}
		}
#endif	
	}while(g_mscd_block_mode && callback_func());
}

/* Flash Identify  */
UINT8 Flash_Identify(UINT8 tLUN)
{
#ifdef TEST_SPI
	if (tLUN==mscdInfo.F_SPI_LUN)
	{
		extern PDISK_T *pDisk_SPI;

		mscdInfo.gTotalSectors_SPI = pDisk_SPI->uDiskSize * 2; 
	}

#endif

#ifdef TEST_SM
    if (tLUN==mscdInfo.F_NAND0_LUN)
    {
		INT nSectorPerPage;

		nSectorPerPage = ptMassNDisk->nPageSize / 512;
       	mscdInfo.gTotalSectors_NAND0 = ptMassNDisk->nZone * (ptMassNDisk->nLBPerZone-1) * ptMassNDisk->nPagePerBlock * nSectorPerPage;

        if (mscdInfo.gTotalSectors_NAND0 < 0)
        {
            mscdInfo.SenseKey = 0x03;
            mscdInfo.ASC= 0x30;
            mscdInfo.ASCQ = 0x01;          
            
            return 0;
        }

    }

    if (tLUN==mscdInfo.F_NAND1_LUN)
    {
		INT nSectorPerPage;

		nSectorPerPage = ptMassNDisk1->nPageSize / 512;
       	mscdInfo.gTotalSectors_NAND1 = ptMassNDisk1->nZone * (ptMassNDisk1->nLBPerZone-1) * ptMassNDisk1->nPagePerBlock * nSectorPerPage;

        if (mscdInfo.gTotalSectors_NAND1 < 0)
        {
            mscdInfo.SenseKey = 0x03;
            mscdInfo.ASC= 0x30;
            mscdInfo.ASCQ = 0x01;          
            
            return 0;
        }

    }

#endif

#ifdef TEST_SD
    if (tLUN==mscdInfo.F_SD0_LUN)
    {
    	int status;
    	if(!bFirstInit0)
    	{	       
			status = sicSdOpen0();
			mscdInfo.gTotalSectors_SD0 = status;
	    }
        if (status == FMI_NO_SD_CARD)
            mscdInfo.card_remove_flag0=1;    
            
        if (mscdInfo.gTotalSectors_SD0 == FMI_NO_SD_CARD)
            mscdInfo.card_remove_flag0=1;  
        if (mscdInfo.gTotalSectors_SD0 > 0) {
	        mscdInfo.card_remove_flag0=0;  
	    }    
        else
        {
			/* cannot read format */
			sicSdClose0();
            mscdInfo.SenseKey = 0x03;
            mscdInfo.ASC= 0x30;
            mscdInfo.ASCQ = 0x01;			
			
            return 0;
        }
        bFirstInit0 = FALSE; 
    }  
    
    if (tLUN==mscdInfo.F_SD1_LUN)
    {
    	int status;
    	if(!bFirstInit1)
    	{	       
		 	status = sicSdOpen1();	        	     	
        	mscdInfo.gTotalSectors_SD1 = status;
	    }
        if (status == FMI_NO_SD_CARD)
            mscdInfo.card_remove_flag1=1;      
        
    
        if (mscdInfo.gTotalSectors_SD1 == FMI_NO_SD_CARD)
            mscdInfo.card_remove_flag1=1;  
        if (mscdInfo.gTotalSectors_SD1 > 0) {
	        mscdInfo.card_remove_flag1=0;  
	    }    
        else
        {
			/* cannot read format */
			sicSdClose1();
            mscdInfo.SenseKey = 0x03;
            mscdInfo.ASC= 0x30;
            mscdInfo.ASCQ = 0x01;			
			
            return 0;
        }
        bFirstInit1 = FALSE; 
    }      
    
    if (tLUN==mscdInfo.F_SD2_LUN)
    {
    	int status;
    	if(!bFirstInit2)
    	{	       
			status = sicSdOpen2();
        	mscdInfo.gTotalSectors_SD2 = status;
	    }
        if (status == FMI_NO_SD_CARD)
            mscdInfo.card_remove_flag2=1;      
        
    
        if (mscdInfo.gTotalSectors_SD2 == FMI_NO_SD_CARD)
            mscdInfo.card_remove_flag2=1;  
        if (mscdInfo.gTotalSectors_SD2 > 0) {
	        mscdInfo.card_remove_flag2=0;  
	    }    
        else
        {
			/* cannot read format */
			sicSdClose2();
            mscdInfo.SenseKey = 0x03;
            mscdInfo.ASC= 0x30;
            mscdInfo.ASCQ = 0x01;			
			
            return 0;
        }
        bFirstInit2 = FALSE; 
    }   
#endif   
	return 0;
}

/* Initial MSC Flash */	
UINT8 mscdFlashInit(NDISK_T *pDisk, INT SDsector)
{
	mscdInfo.F_SD0_LUN = 0xFF;
  	mscdInfo.F_SD1_LUN = 0xFF;   
 	mscdInfo.F_SD2_LUN = 0xFF;
    mscdInfo.F_NAND0_LUN = 0xFF;
	mscdInfo.F_NAND1_LUN = 0xFF;  
	mscdInfo.F_RAM_LUN = 0xFF;    
	mscdInfo.F_CDROM_LUN = 0xFF;		
	mscdInfo.F_SPI_LUN = 0xFF;		
    mscdInfo.Mass_LUN = 0;

#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
	    mscdInfo.F_NAND0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
   		mscdInfo.F_NAND1_LUN = mscdInfo.Mass_LUN;
	    mscdInfo.Mass_LUN++;
	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
	    mscdInfo.F_SD0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
	    mscdInfo.F_SD1_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }    
    
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
	    mscdInfo.F_SD2_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    } 
#endif
#ifdef TEST_SPI
    mscdInfo.F_SPI_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    Flash_Identify(mscdInfo.F_SPI_LUN);
#endif

#ifdef TEST_RAM
	mscdInfo.F_RAM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
  	DRAM_Identify(RAMDISK_SIZE);
    format();	
#endif    

#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
		ptMassNDisk = (NDISK_T *)pDisk;
    	if (!Flash_Identify(mscdInfo.F_NAND0_LUN))
        	; //return 0;
    }
    
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
		ptMassNDisk1 = (NDISK_T *)pDisk;
	    if (!Flash_Identify(mscdInfo.F_NAND1_LUN))
	        ; //return 0;
	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD0 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD0_LUN))
				;//return 0;
		}
		else
			bFirstInit0 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD1 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD1_LUN))
				;//return 0;
		}
		else
			bFirstInit1 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD2 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD2_LUN))
				;//return 0;
		}
		else
			bFirstInit2 = FALSE;
	}	
#endif
    return 1;
}
/* Initial MSC Flash */	
UINT8 mscdFlashInitNAND(NDISK_T *pDisk,NDISK_T *pDisk1,NDISK_T *pDisk2, INT SDsector)
{
	mscdInfo.F_SD0_LUN = 0xFF;
  	mscdInfo.F_SD1_LUN = 0xFF;   
 	mscdInfo.F_SD2_LUN = 0xFF;
    mscdInfo.F_NAND0_LUN = 0xFF;
	mscdInfo.F_NAND1_LUN = 0xFF; 
	mscdInfo.F_RAM_LUN = 0xFF;    
	mscdInfo.F_CDROM_LUN = 0xFF;		
	mscdInfo.F_SPI_LUN = 0xFF;	
    mscdInfo.Mass_LUN = 0;
	
#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
	    mscdInfo.F_NAND0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
   		mscdInfo.F_NAND1_LUN = mscdInfo.Mass_LUN;
	    mscdInfo.Mass_LUN++;
	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
	    mscdInfo.F_SD0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
	    mscdInfo.F_SD1_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }    
    
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
	    mscdInfo.F_SD2_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    } 
#endif
#ifdef TEST_SPI
    mscdInfo.F_SPI_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    Flash_Identify(mscdInfo.F_SPI_LUN);
#endif
#ifdef TEST_RAM
	mscdInfo.F_RAM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
  	DRAM_Identify(RAMDISK_SIZE);
    format();	
#endif    

#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
		ptMassNDisk = (NDISK_T *)pDisk;
    	if (!Flash_Identify(mscdInfo.F_NAND0_LUN))
        	; //return 0;
	}
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{	
		ptMassNDisk1 = (NDISK_T *)pDisk1;
    	if (!Flash_Identify(mscdInfo.F_NAND1_LUN))
        	; //return 0;
    }
#endif
#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD0 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD0_LUN))
				;//return 0;
		}
		else
			bFirstInit0 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD1 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD1_LUN))
				;//return 0;
		}
		else
			bFirstInit1 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD2 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD2_LUN))
				;//return 0;
		}
		else
			bFirstInit2 = FALSE;
	}	
#endif
    return 1;
}

/* Initial MSC Flash */	
UINT8 mscdFlashInitExtend(NDISK_T *pDisk,NDISK_T *pDisk1,NDISK_T *pDisk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize)
{
	mscdInfo.F_SD0_LUN = 0xFF;
  	mscdInfo.F_SD1_LUN = 0xFF;   
 	mscdInfo.F_SD2_LUN = 0xFF;
    mscdInfo.F_NAND0_LUN = 0xFF;
	mscdInfo.F_NAND1_LUN = 0xFF;    
	mscdInfo.F_RAM_LUN = 0xFF;    
	mscdInfo.F_CDROM_LUN = 0xFF;
	mscdInfo.F_SPI_LUN = 0xFF;		
    mscdInfo.Mass_LUN = 0;
	
#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
	    mscdInfo.F_NAND0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
   		mscdInfo.F_NAND1_LUN = mscdInfo.Mass_LUN;
  		mscdInfo.Mass_LUN++;
  	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
	    mscdInfo.F_SD0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
	    mscdInfo.F_SD1_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }    
    
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
	    mscdInfo.F_SD2_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    } 
#endif
#ifdef TEST_SPI
    mscdInfo.F_SPI_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    Flash_Identify(mscdInfo.F_SPI_LUN);
#endif
#ifdef TEST_RAM
	mscdInfo.F_RAM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
  	DRAM_Identify(RamSize);
    format();	
#endif    

#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
		ptMassNDisk = (NDISK_T *)pDisk;
    	if (!Flash_Identify(mscdInfo.F_NAND0_LUN))
        	; //return 0;
    }
    
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
		ptMassNDisk1 = (NDISK_T *)pDisk1;
	    if (!Flash_Identify(mscdInfo.F_NAND1_LUN))
	        ; //return 0;
	}
#endif
#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
		if(SDsector0 > 0)
		{			
			mscdInfo.gTotalSectors_SD0 = SDsector0;
	    	if (!Flash_Identify(mscdInfo.F_SD0_LUN))
				;//return 0;
		}
		else
			bFirstInit0 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
		if(SDsector1 > 0)
		{			
			mscdInfo.gTotalSectors_SD1 = SDsector1;
	    	if (!Flash_Identify(mscdInfo.F_SD1_LUN))
				;//return 0;
		}
		else
			bFirstInit1 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
		if(SDsector2 > 0)
		{			
			mscdInfo.gTotalSectors_SD2 = SDsector2;
	    	if (!Flash_Identify(mscdInfo.F_SD2_LUN))
				;//return 0;
		}
		else
			bFirstInit2 = FALSE;
	}		

#endif
    return 1;
}

#ifdef TEST_CDROM 

/* Initial MSC Flash */	
UINT8 mscdFlashInitExtendCDROM(NDISK_T *pDisk,NDISK_T *pDisk1,NDISK_T *pDisk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize, PFN_MSCD_CDROM_CALLBACK pfnCallBack ,INT CdromSizeInByte)
{
	mscdInfo.F_SD0_LUN = 0xFF;
  	mscdInfo.F_SD1_LUN = 0xFF;   
 	mscdInfo.F_SD2_LUN = 0xFF;
    mscdInfo.F_NAND0_LUN = 0xFF;
	mscdInfo.F_NAND1_LUN = 0xFF;    
	mscdInfo.F_RAM_LUN = 0xFF;    
	mscdInfo.F_CDROM_LUN = 0xFF;
	mscdInfo.F_SPI_LUN = 0xFF;		
    mscdInfo.Mass_LUN = 0;
	
#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
	    mscdInfo.F_NAND0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
   		mscdInfo.F_NAND1_LUN = mscdInfo.Mass_LUN;
  		mscdInfo.Mass_LUN++;
  	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
	    mscdInfo.F_SD0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
	    mscdInfo.F_SD1_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }    
    
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
	    mscdInfo.F_SD2_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    } 
#endif
#ifdef TEST_SPI
    mscdInfo.F_SPI_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    Flash_Identify(mscdInfo.F_SPI_LUN);
#endif
#ifdef TEST_RAM
	mscdInfo.F_RAM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
  	DRAM_Identify(RamSize);
    format();	
#endif    
#ifdef TEST_CDROM
    mscdInfo.F_CDROM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    pfnMSC_CDROM_CALLBACK = pfnCallBack;
    mscdInfo.gTotalSectors_CDROM = CdromSizeInByte / 2048;
#endif  
#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
		ptMassNDisk = (NDISK_T *)pDisk;
    	if (!Flash_Identify(mscdInfo.F_NAND0_LUN))
        	; //return 0;
    }
    
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
		ptMassNDisk1 = (NDISK_T *)pDisk1;
	    if (!Flash_Identify(mscdInfo.F_NAND1_LUN))
	        ; //return 0;
	}
#endif
#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
		if(SDsector0 > 0)
		{			
			mscdInfo.gTotalSectors_SD0 = SDsector0;
	    	if (!Flash_Identify(mscdInfo.F_SD0_LUN))
				;//return 0;
		}
		else
			bFirstInit0 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
		if(SDsector1 > 0)
		{			
			mscdInfo.gTotalSectors_SD1 = SDsector1;
	    	if (!Flash_Identify(mscdInfo.F_SD1_LUN))
				;//return 0;
		}
		else
			bFirstInit1 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
		if(SDsector2 > 0)
		{			
			mscdInfo.gTotalSectors_SD2 = SDsector2;
	    	if (!Flash_Identify(mscdInfo.F_SD2_LUN))
				;//return 0;
		}
		else
			bFirstInit2 = FALSE;
	}		

#endif
    return 1;
}
UINT8 mscdFlashInitCDROM(NDISK_T *pDisk, INT SDsector, PFN_MSCD_CDROM_CALLBACK pfnCallBack ,INT CdromSizeInByte)
{
	mscdInfo.F_SD0_LUN = 0xFF;
  	mscdInfo.F_SD1_LUN = 0xFF;   
 	mscdInfo.F_SD2_LUN = 0xFF;
    mscdInfo.F_NAND0_LUN = 0xFF;
	mscdInfo.F_NAND1_LUN = 0xFF;    
	mscdInfo.F_RAM_LUN = 0xFF;    
	mscdInfo.F_CDROM_LUN = 0xFF;	
	mscdInfo.F_SPI_LUN = 0xFF;		
    mscdInfo.Mass_LUN = 0;		

#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
	    mscdInfo.F_NAND0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
   		mscdInfo.F_NAND1_LUN = mscdInfo.Mass_LUN;
	    mscdInfo.Mass_LUN++;
	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
	    mscdInfo.F_SD0_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
	    mscdInfo.F_SD1_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    }    
    
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
	    mscdInfo.F_SD2_LUN = mscdInfo.Mass_LUN;
    	mscdInfo.Mass_LUN++;
    } 
#endif
#ifdef TEST_SPI
    mscdInfo.F_SPI_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    Flash_Identify(mscdInfo.F_SPI_LUN);
#endif
#ifdef TEST_RAM
	mscdInfo.F_RAM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
  	DRAM_Identify(RAMDISK_SIZE);
    format();	
#endif    

#ifdef TEST_CDROM
    mscdInfo.F_CDROM_LUN = mscdInfo.Mass_LUN;
    mscdInfo.Mass_LUN++;
    pfnMSC_CDROM_CALLBACK = pfnCallBack;
    mscdInfo.gTotalSectors_CDROM = CdromSizeInByte / 2048;
#endif    

#ifdef TEST_SM
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS0)
	{
		ptMassNDisk = (NDISK_T *)pDisk;
    	if (!Flash_Identify(mscdInfo.F_NAND0_LUN))
        	; //return 0;
    }
    
	if(g_MSC_NAND_CS_ENABLE & MSC_NAND_CS1)
	{
		ptMassNDisk1 = (NDISK_T *)pDisk;
	    if (!Flash_Identify(mscdInfo.F_NAND1_LUN))
	        ; //return 0;
	}
#endif

#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD0 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD0_LUN))
				;//return 0;
		}
		else
			bFirstInit0 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD1 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD1_LUN))
				;//return 0;
		}
		else
			bFirstInit1 = FALSE;
	}
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
	{
		if(SDsector > 0)
		{			
			mscdInfo.gTotalSectors_SD2 = SDsector;
	    	if (!Flash_Identify(mscdInfo.F_SD2_LUN))
				;//return 0;
		}
		else
			bFirstInit2 = FALSE;
	}	
#endif
    return 1;
}
#endif	
VOID mscdSdEnable(UINT32 u32Enable)
{
	g_MSC_SD_PORT_ENABLE = u32Enable;
}
VOID mscdNandEnable(UINT32 u32Enable)
{
	g_MSC_NAND_CS_ENABLE = u32Enable;
}

#ifdef TEST_SD
VOID mscdSdWriteProtectEnable(UINT32 u32Enable)
{
	u32SdWriteProtectEnable = u32Enable;
}

VOID mscdSdCardDectEnable(UINT32 u32Enable)
{
	u32SdCardDetectEnable = u32Enable;
}

VOID mscdSdUserWriteProtectPin(UINT32 u32SdPort, BOOL bEnable, UINT32 u32GpioPort, UINT32 u32GpioPin)
{
	switch(u32SdPort)
	{
		case MSC_SD_PORT0:
				u32Sd0UserWriteProtectPort = u32GpioPort;
				u32Sd0UserWriteProtectPin = u32GpioPin;		
				break;
		case MSC_SD_PORT1:
				u32Sd1UserWriteProtectPort = u32GpioPort;
				u32Sd1UserWriteProtectPin = u32GpioPin;		
				break;
		case MSC_SD_PORT2:
				u32Sd2UserWriteProtectPort = u32GpioPort;
				u32Sd2UserWriteProtectPin = u32GpioPin;		
				break;			
	}
	if(bEnable)
		u32SdUserWriteProtect = u32SdUserWriteProtect | u32SdPort;
	else
		u32SdUserWriteProtect = u32SdUserWriteProtect & ~u32SdPort; 
	
}

VOID mscdSdUserCardDetectPin(UINT32 u32SdPort, BOOL bEnable, UINT32 u32GpioPort, UINT32 u32GpioPin)
{
	switch(u32SdPort)
	{
		case MSC_SD_PORT0:
				u32Sd0UserCardDetectPort = u32GpioPort;
				u32Sd0UserCardDetectPin = u32GpioPin;		
				break;
		case MSC_SD_PORT1:
				u32Sd1UserCardDetectPort = u32GpioPort;
				u32Sd1UserCardDetectPin = u32GpioPin;		
				break;
		case MSC_SD_PORT2:
				u32Sd2UserCardDetectPort = u32GpioPort;
				u32Sd2UserCardDetectPin = u32GpioPin;		
				break;			
	}
	if(bEnable)
		u32SdUserCardDetect = u32SdUserCardDetect | u32SdPort;
	else
		u32SdUserCardDetect = u32SdUserCardDetect & ~u32SdPort; 
	
}
#endif

/* MSC Init */
VOID mscdInit(void)
{
	sysprintf("N3292 MSC Library (%s)\n",DATA_CODE);
	/* Set Endpoint map */
	usbdInfo.i32EPA_Num = 1;	/* Endpoint 1 */
	usbdInfo.i32EPB_Num = 2;	/* Endpoint 2 */
	usbdInfo.i32EPC_Num = -1;	/* Not use */
	usbdInfo.i32EPD_Num = -1;	/* Not use */
	
	/* Set Callback Function */
//	usbdInfo.pfnDMACompletion =	MSC_DMA_Completion;
	usbdInfo.pfnClassDataINCallBack = MSC_ClassDataIn;
	usbdInfo.pfnClassDataOUTCallBack = MSC_ClassDataOut;	
	usbdInfo.pfnReset  = MSC_Reset;
	usbdInfo.pfnEPBCallBack = MSC_EPB_CallBack;
	usbdInfo.pfnEPACallBack = MSC_EPA_CallBack;
	
	/* Set MSC initialize function */
	usbdInfo.pfnFullSpeedInit = mscdFullSpeedInit;
	usbdInfo.pfnHighSpeedInit = mscdHighSpeedInit;
	
	/* Set Descriptor pointer */
	usbdInfo.pu32DevDescriptor = (PUINT32) &MSC_DeviceDescriptor;
	usbdInfo.pu32QulDescriptor = (PUINT32) &MSC_QualifierDescriptor;
	usbdInfo.pu32HSConfDescriptor = (PUINT32) &MSC_ConfigurationBlock;
	usbdInfo.pu32FSConfDescriptor = (PUINT32) &MSC_ConfigurationBlockFull;
	usbdInfo.pu32HOSConfDescriptor = (PUINT32) &MSC_HOSConfigurationBlock;	
	usbdInfo.pu32FOSConfDescriptor = (PUINT32) &MSC_FOSConfigurationBlock;	
	usbdInfo.pu32StringDescriptor[0] = (PUINT32) &MSC_StringDescriptor0;
	usbdInfo.pu32StringDescriptor[1] = (PUINT32) &MSC_StringDescriptor1;
	usbdInfo.pu32StringDescriptor[2] = (PUINT32) &MSC_StringDescriptor2;
	usbdInfo.pu32StringDescriptor[3] = (PUINT32) &MSC_StringDescriptor3;
	
	/* Set Descriptor length */	
	usbdInfo.u32DevDescriptorLen =  MSC_DEVICE_DSCPT_LEN;
	usbdInfo.u32HSConfDescriptorLen =  MSC_CONFIG_DSCPT_LEN;
	usbdInfo.u32FSConfDescriptorLen =  MSC_CONFIG_DSCPT_LEN;	
	usbdInfo.u32HOSConfDescriptorLen =  MSC_CONFIG_DSCPT_LEN;		
	usbdInfo.u32FOSConfDescriptorLen =  MSC_CONFIG_DSCPT_LEN;		
	usbdInfo.u32StringDescriptorLen[0] =  MSC_STR0_DSCPT_LEN;
	usbdInfo.u32StringDescriptorLen[1] = MSC_StringDescriptor1[0] = sizeof(MSC_StringDescriptor1);
	usbdInfo.u32StringDescriptorLen[2] = MSC_StringDescriptor2[0] = sizeof(MSC_StringDescriptor2);
	usbdInfo.u32StringDescriptorLen[3] = MSC_StringDescriptor3[0] = sizeof(MSC_StringDescriptor3);		
	usbdInfo.u32QulDescriptorLen =  MSC_QUALIFIER_DSCPT_LEN;
	
	/* Set MSC property */
	mscdInfo.SizePerSector = 512;
	mscdInfo.USBD_DMA_LEN = 0x20000;
	mscdInfo.Mass_LUN=0;
	mscdInfo.F_SD0_LUN = 0xFF;
  	mscdInfo.F_SD1_LUN = 0xFF;   
 	mscdInfo.F_SD2_LUN = 0xFF;
    mscdInfo.F_NAND0_LUN = 0xFF;
	mscdInfo.F_NAND1_LUN = 0xFF;    
	mscdInfo.F_RAM_LUN = 0xFF;  
	mscdInfo.F_SPI_LUN = 0xFF;  
	mscdInfo.F_CDROM_LUN = 0xFF;	
	mscdInfo.gTotalSectors_RAM = 16000;
#ifndef __ARRAY_BUFFER__	
	g_u32MscMassAddr = (UINT32) malloc(sizeof(UINT8) * 1055);
	mscdInfo.Mass_Base_Addr = (g_u32MscMassAddr + 0x1F) & ~0x1F;
#else
	mscdInfo.Mass_Base_Addr = (UINT32)MSC_CMD_BUFFER;
#endif	
	mscdInfo.Mass_Base_Addr |= 0x80000000;
	
#ifndef __ARRAY_BUFFER__	
	g_u32MscStorageAddr = (UINT32) malloc(sizeof(UINT8) * 65567);
	mscdInfo.Storage_Base_Addr = (g_u32MscStorageAddr + 0x1F) & ~0x1F;
#else
	mscdInfo.Storage_Base_Addr = (UINT32)MSC_DATA_BUFFER;
#endif		
	mscdInfo.Storage_Base_Addr |= 0x80000000;
	mscdInfo.SenseKey = 0x00;
	mscdInfo.ASC= 0x00;
	mscdInfo.ASCQ=0x00;	
	
}

VOID mscdDeinit(void)
{
#ifdef TEST_SD
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT0)
		sicSdClose0();
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT1)
		sicSdClose1();
	if(g_MSC_SD_PORT_ENABLE & MSC_SD_PORT2)
		sicSdClose2();
#endif

#ifdef TEST_RAM
	free((UINT8*)g_u32MSCRamDiskAddr);
#endif

#ifndef __ARRAY_BUFFER__
	free((UINT8*)g_u32MscMassAddr);
	free((UINT8*)g_u32MscStorageAddr);	
#endif	
}

/* RAM Disk Only */
#ifdef TEST_RAM
void DRAM_Identify(UINT8 cap)
{
	if (cap==MSC_RAMDISK_1M) 
	{
		mscdInfo.gTotalSectors_RAM=2000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_1M;
		mscdInfo.DDB.NumCyl=250;
		mscdInfo.DDB.NumHead=4;
		mscdInfo.DDB.NumSector=8;		
	}
	else if (cap==MSC_RAMDISK_4M) 
	{
		mscdInfo.gTotalSectors_RAM=8000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_4M;
		mscdInfo.DDB.NumCyl=250;
		mscdInfo.DDB.NumHead=4;
		mscdInfo.DDB.NumSector=8;
	}
	else if (cap==MSC_RAMDISK_8M)
	{
		mscdInfo.gTotalSectors_RAM=16000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_8M;
		mscdInfo.DDB.NumCyl=250;
		mscdInfo.DDB.NumHead=4;
		mscdInfo.DDB.NumSector=16;
	}
	else if (cap==MSC_RAMDISK_16M) 
	{
		mscdInfo.gTotalSectors_RAM=32000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_16M;
		mscdInfo.DDB.NumCyl=500;
		mscdInfo.DDB.NumHead=4;
		mscdInfo.DDB.NumSector=16;
	}
	else if (cap==MSC_RAMDISK_32M)
	{
		mscdInfo.gTotalSectors_RAM=64000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_32M;
		mscdInfo.DDB.NumCyl=500;
		mscdInfo.DDB.NumHead=8;
		mscdInfo.DDB.NumSector=16;
	}
	else if (cap==MSC_RAMDISK_64M)
	{
		mscdInfo.gTotalSectors_RAM=128000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_64M;
		mscdInfo.DDB.NumCyl=500;
		mscdInfo.DDB.NumHead=8;
		mscdInfo.DDB.NumSector=32;
	}
	else if (cap==MSC_RAMDISK_128M)
	{
		mscdInfo.gTotalSectors_RAM=2560000;
		mscdInfo.DDB.capacity = MSC_RAMDISK_128M;
		mscdInfo.DDB.NumCyl=500;
		mscdInfo.DDB.NumHead=16;
		mscdInfo.DDB.NumSector=32;
	}
	g_u32MSCRamDiskAddr = (UINT32) malloc(sizeof(UINT8) * (mscdInfo.gTotalSectors_RAM*512+0x1F));
	mscdInfo.Storage_Base_Addr_RAMDISK = (g_u32MSCRamDiskAddr + 0x1F) & ~0x1F;
	mscdInfo.Storage_Base_Addr_RAMDISK |= 0x80000000;
	
}
UINT8 format(void)
{
	UINT8 i;
	UINT8 *bp,fat16=0;
	UINT32 totalcluster;

	memset(Flash_Buffer, 0, mscdInfo.SizePerSector);
	Flash_Buffer[0x1be] = 0x80;
	Flash_Buffer[0x1bf] = StartSect[mscdInfo.DDB.capacity]/mscdInfo.DDB.NumSector;
	Flash_Buffer[0x1c0] = (StartSect[mscdInfo.DDB.capacity]%mscdInfo.DDB.NumSector)+1;
	Flash_Buffer[0x1c1] = 0x00; 

	if (mscdInfo.DDB.capacity>=7)
	{
		fat16=1;
		Flash_Buffer[0x1c2] = 0x06;
	}
	else
		Flash_Buffer[0x1c2] = 0x01;

	Flash_Buffer[0x1c3] = mscdInfo.DDB.NumHead-1;
	Flash_Buffer[0x1c4] = ((mscdInfo.DDB.NumCyl & 0x300)>>2)+mscdInfo.DDB.NumSector;
	Flash_Buffer[0x1c5] = (mscdInfo.DDB.NumCyl & 0xff)-1;
	Flash_Buffer[0x1c6] = StartSect[mscdInfo.DDB.capacity];

	mscdInfo.DDB.partition_size = mscdInfo.gTotalSectors_RAM - StartSect[mscdInfo.DDB.capacity];

	totalcluster = (UINT16)mscdInfo.DDB.NumHead*mscdInfo.DDB.NumCyl;
	bp = Flash_Buffer+0x1ca;
	put_uint32((UINT32)mscdInfo.DDB.partition_size, (UINT8 **)&bp);

	Flash_Buffer[510] = 0x55;
	Flash_Buffer[511] = 0xaa;

	Write_Sector(0, Flash_Buffer);

	if (!WriteBootSector(fat16))
		return 0;

	memset(Flash_Buffer, 0, mscdInfo.SizePerSector);
	for (i=0;i<FATSects[mscdInfo.DDB.capacity]-1;i++)
	{
		Write_Sector((UINT32)(StartSect[mscdInfo.DDB.capacity]+2+i),Flash_Buffer);
		Write_Sector((UINT32)(StartSect[mscdInfo.DDB.capacity]+FATSects[mscdInfo.DDB.capacity]+2+i),Flash_Buffer);
	}

	for (i=0;i<16;i++)      
	{
		Write_Sector((UINT32)(StartSect[mscdInfo.DDB.capacity]+FATSects[mscdInfo.DDB.capacity]*2+i+1),Flash_Buffer);
	}

	memset(Flash_Buffer, 0, mscdInfo.SizePerSector);
	Flash_Buffer[0]=0xf8;

	Flash_Buffer[1] = 0xff;
	Flash_Buffer[2] = 0xff;
	if (fat16==1)
		Flash_Buffer[3] = 0xff;

	Write_Sector((UINT32)(StartSect[mscdInfo.DDB.capacity]+1),Flash_Buffer);
	Write_Sector((UINT32)(StartSect[mscdInfo.DDB.capacity]+1+FATSects[mscdInfo.DDB.capacity]),Flash_Buffer);
	return 1;
}

UINT8 WriteBootSector(UINT8 fat16)
{
	UINT8 *bp;
	UINT8 i;

	memset(Flash_Buffer, 0, mscdInfo.SizePerSector);
	for (i=0;i<0x13;i++)
		Flash_Buffer[i] = PartBootSector[i];

	if ((mscdInfo.DDB.capacity==2) || (mscdInfo.DDB.capacity==3))
		Flash_Buffer[13] = 16;
	else
		Flash_Buffer[13] = 32;

	bp = Flash_Buffer+0x13;
	if (mscdInfo.DDB.partition_size < 0x10000)
		put_uint16(mscdInfo.DDB.partition_size, (UINT8 **)&bp);
	else
		bp = bp+2;

	Flash_Buffer[0x15] = 0xf8;
	bp++;

	put_uint16(FATSects[mscdInfo.DDB.capacity], (UINT8 **)&bp);
	put_uint16(mscdInfo.DDB.NumSector, (UINT8 **)&bp);
	put_uint16(mscdInfo.DDB.NumHead, (UINT8 **)&bp);

	Flash_Buffer[0x1c] = StartSect[mscdInfo.DDB.capacity];

	if (mscdInfo.DDB.partition_size >= 0x10000)
	{
		bp = Flash_Buffer+0x20;
		put_uint32(mscdInfo.DDB.partition_size, (UINT8 **)&bp);
	}

	Flash_Buffer[0x36] = 'F';
	Flash_Buffer[0x37] = 'A';
	Flash_Buffer[0x38] = 'T';
	Flash_Buffer[0x39] = '1';
	if (fat16==1)
		Flash_Buffer[0x3a] = '6';
	else
		Flash_Buffer[0x3a] = '2';
	Flash_Buffer[0x3b] = ' ';
	Flash_Buffer[0x3c] = ' ';

	Flash_Buffer[510] = 0x55;
	Flash_Buffer[511] = 0xaa;

	Write_Sector((UINT32)(StartSect[mscdInfo.DDB.capacity]), Flash_Buffer);
	return 1;
}

void put_uint32(UINT32 value, UINT8 **p)
{
	union
	{
		UINT8 c[4];
		UINT32 i32; 
	}un_32;

	un_32.i32 = value;
	**p = un_32.c[0];
	(*p)[1] = un_32.c[1];
	(*p)[2] = un_32.c[2];
	(*p)[3] = un_32.c[3];
}

void put_uint16(UINT16 value, UINT8 **p)
{
	*(*p)++ = (UINT8)(value & 0xff);
	*(*p)++ = (UINT8)(value >> 8);
}

UINT8 Write_Sector(UINT32 sector,UINT8 *buffer)
{
	UINT16 tmpv;
	UINT32 tmpv1;
	UINT8 gValue;

	tmpv1 = mscdInfo.Storage_Base_Addr_RAMDISK+(sector<<9);
	for (tmpv=0;tmpv<mscdInfo.SizePerSector;tmpv++)
	{
		gValue=*buffer++;
		outp8(tmpv1,gValue);
		tmpv1++;
	}
	return 1;
}
#endif



#ifdef TEST_CDROM
static UINT8 Command_51_01[8] = {
 	0x00, 0x20, 0x01, 0x01, 0x02, 0x02, 0x02, 0x80 };

VOID mscdCommand_51(void)
{
	UINT8 j;
    UINT32 tmpval=mscdInfo.Mass_Base_Addr;
	if (CBW_In.CBWD.dCBWDataTferLh.c32 > 8)
	{
		for (j = 0; j<8; j++)
			writeb(tmpval+j,Command_51_01[j]);
		mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , 8);
	}
	else
	{
		for (j = 0; j<CBW_In.CBWD.dCBWDataTferLh.c32; j++)
			writeb(tmpval+j, Command_51_01[j]);
		mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , CBW_In.CBWD.dCBWDataTferLh.c32);
	}
}

static UINT8 Command_4A_01[8] = {
 	0x00, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 };

VOID mscdCommand_4A(void)
{
	UINT8 j;
	UINT32 tmpval=mscdInfo.Mass_Base_Addr;
	if (CBW_In.CBWD.dCBWDataTferLh.c32 > 8)
	{
		for (j = 0; j<8; j++)
			writeb(tmpval+j,Command_4A_01[j]);
		mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr ,8);	
	}
	else
	{
		for (j = 0; j<CBW_In.CBWD.dCBWDataTferLh.c32; j++)
			writeb(tmpval+j,Command_4A_01[j]);
		mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr ,CBW_In.CBWD.dCBWDataTferLh.c32);
	}
}

static UINT8 Command_A4_01[8] = {
 	0x00, 0x06, 0x00, 0x00, 0x64, 0xFE, 0x01, 0x00 };

VOID mscdCommand_A4(void)
{
	UINT8 j;
	UINT32 tmpval=mscdInfo.Mass_Base_Addr;
	if (CBW_In.CBWD.dCBWDataTferLh.c32 > 8)
	{
		for (j = 0; j<8; j++)
			writeb(tmpval+j,Command_A4_01[j]);
		mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr ,8);
	}
	else
	{
		for (j = 0; j<CBW_In.CBWD.dCBWDataTferLh.c32; j++)
			writeb(tmpval+j,Command_A4_01[j]);
		mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , CBW_In.CBWD.dCBWDataTferLh.c32);
	}
}


static UINT8 Command_46_00[328] = {
 	0x00, 0x00, 0x01, 0x44, 0x00, 0x00, 0x00, 0x09,
 	0x00, 0x00, 0x03, 0x34, 0x00, 0x2B, 0x00, 0x00,
 	0x00, 0x1B, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00,
 	0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,
 	0x00, 0x13, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00,
 	0x00, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
 	0x00, 0x0A, 0x00, 0x00, 0x00, 0x09, 0x01, 0x00,
 	0x00, 0x08, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
 	0x00, 0x01, 0x03, 0x04, 0x00, 0x00, 0x00, 0x02,
 	0x00, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00,
 	0x00, 0x03, 0x03, 0x04, 0x29, 0x00, 0x00, 0x00,
 	0x00, 0x04, 0x04, 0x04, 0x02, 0x00, 0x00, 0x00,
 	0x00, 0x10, 0x01, 0x08, 0x00, 0x00, 0x08, 0x00,
 	0x00, 0x01, 0x01, 0x00, 0x00, 0x1D, 0x01, 0x00,
 	0x00, 0x1E, 0x05, 0x04, 0x03, 0x00, 0x00, 0x00,
 	0x00, 0x1F, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00,
 	0x00, 0x20, 0x04, 0x0C, 0x00, 0x00, 0x00, 0x00,
 	0x00, 0x00, 0x08, 0x00, 0x00, 0x01, 0x01, 0x00,
 	0x00, 0x21, 0x0D, 0x08, 0x3D, 0x01, 0x01, 0x01,
 	0x07, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00,
 	0x00, 0x24, 0x04, 0x04, 0x80, 0x00, 0x00, 0x00,
 	0x00, 0x26, 0x00, 0x00, 0x00, 0x28, 0x00, 0x04,
 	0x01, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x04,
 	0x01, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x04,
 	0x01, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x04,
 	0x03, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x09, 0x04,
 	0x46, 0x00, 0x3D, 0x01, 0x00, 0x2E, 0x05, 0x04,
 	0x66, 0x00, 0x24, 0x00, 0x00, 0x2F, 0x08, 0x04,
 	0x4E, 0x00, 0x00, 0x00, 0x00, 0x37, 0x00, 0x04,
	0x00, 0x0F, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00,
	0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};


static UINT8 Command_46_1E[8] = {
 	0x00, 0x1E, 0x05, 0x04, 0x03, 0x00, 0x00, 0x00};

static UINT8 Command_46_1F[8] = {
 	0x00, 0x1F, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};

static UINT8 Command_46_20[16] = {
 	0x00, 0x20, 0x04, 0x0C, 0x00, 0x00, 0x00, 0x00,
 	0x00, 0x00, 0x08, 0x00, 0x00, 0x01, 0x01, 0x00};

static UINT8 Command_46_21[12] = {
 	0x00, 0x21, 0x0D, 0x08, 0x3D, 0x01, 0x01, 0x01,
 	0x07, 0x00, 0x00, 0x00};

static UINT8 Command_46_23[4] = {
 	0x00, 0x23, 0x00, 0x00};

static UINT8 Command_46_24[8] = {
 	0x00, 0x24, 0x04, 0x04, 0x80, 0x00, 0x00, 0x00};

static UINT8 Command_46_26[4] = {
 	0x00, 0x26, 0x00, 0x00};

static UINT8 Command_46_2A[8] = {
 	0x00, 0x2A, 0x00, 0x04,	0x01, 0x00, 0x00, 0x00};

static UINT8 Command_46_2B[8] = {
 	0x00, 0x2B, 0x00, 0x04,	0x01, 0x00, 0x00, 0x00};

static UINT8 Command_46_2D[8] = {
 	0x00, 0x2D, 0x09, 0x04,	0x46, 0x00, 0x3D, 0x01};

static UINT8 Command_46_2E[8] = {
 	0x00, 0x2E, 0x05, 0x04,	0x66, 0x00, 0x24, 0x00};

static UINT8 Command_46_2F[8] = {
 	0x00, 0x2F, 0x08, 0x04,	0x4E, 0x00, 0x00, 0x00};

static UINT8 Command_46_3B[8] = {
 	0x00, 0x3B, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00};

static UINT8 Command_46_01[176] = {
 	0x00, 0x00, 0x00, 0xAC, 0x00, 0x00, 0x00, 0x09,
	0x00, 0x00, 0x03, 0x34, 0x00, 0x2B, 0x00, 0x00,
 	0x00, 0x1B, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00,
 	0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,
 	0x00, 0x13, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00,
 	0x00, 0x11, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
 	0x00, 0x0A, 0x00, 0x00, 0x00, 0x09, 0x01, 0x00,
 	0x00, 0x08, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
 	0x00, 0x01, 0x03, 0x04, 0x00, 0x00, 0x00, 0x02,
 	0x00, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00,
 	0x00, 0x03, 0x03, 0x04, 0x29, 0x00, 0x00, 0x00,
 	0x00, 0x10, 0x01, 0x08, 0x00, 0x00, 0x08, 0x00,
 	0x00, 0x01, 0x01, 0x00, 0x00, 0x1D, 0x01, 0x00,
 	0x00, 0x1E, 0x05, 0x04, 0x03, 0x00, 0x00, 0x00,
 	0x00, 0x21, 0x0D, 0x08, 0x3D, 0x01, 0x01, 0x01,
 	0x07, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x09, 0x04,
 	0x46, 0x00, 0x3D, 0x01, 0x00, 0x2E, 0x05, 0x04,
 	0x66, 0x00, 0x24, 0x00,	0x01, 0x00, 0x03, 0x00,
 	0x01, 0x05, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00,
 	0x01, 0x07, 0x0D, 0x04, 0x1F, 0x00, 0x00, 0x00,
 	0x01, 0x08, 0x03, 0x0C, 0x54, 0x43, 0x4A, 0x49,
 	0x41, 0x34, 0x30, 0x31, 0x34, 0x42, 0x36, 0x33
};

	
static __inline UINT16 get_be16(UINT8 *buf)
{
	return (((UINT16) buf[0] << 8) | ((UINT16) buf[1]));
}


VOID mscdCommand_46(void)
{
	UINT32 len;
	UINT8 *buff;
	
	len = CBW_In.CBWD.dCBWDataTferLh.c32;
	buff = (UINT8 *)mscdInfo.Mass_Base_Addr;
	
	if (!bIsDeviceReady)
	{
		memset((char *)buff, 0, len);
		return;
	}

	if (CBW_In.CBWD.LUN == 0x02)
	{
		memset(buff, 0x0, len);
		if (get_be16(&CBW_In.CBWD.LBA.d[0]) == 0x0000)
			memcpy((char *)buff, &Command_46_00[0], len);
		else
		{
			memcpy((char *)buff, &Command_46_00[0], 8);
			buff += 8;
			switch(get_be16(&CBW_In.CBWD.LBA.d[0]))
			{
				case 0x001E:
					memcpy((char *)buff, &Command_46_1E[0], 8);
					break;
				case 0x001F:
					memcpy((char *)buff, &Command_46_1F[0], 8);
					break;
				case 0x0020:
					memcpy((char *)buff, &Command_46_20[0], 8);
					break;
				case 0x0021:
					memcpy((char *)buff, &Command_46_21[0], 8);
					break;
				case 0x0023:
					memcpy((char *)buff, &Command_46_23[0], 4);
					break;
				case 0x0024:
					memcpy((char *)buff, &Command_46_24[0], 8);
					break;
				case 0x0026:
					memcpy((char *)buff, &Command_46_26[0], 4);
					break;
				case 0x002A:
					memcpy((char *)buff, &Command_46_2A[0], 8);
					break;
				case 0x002B:
					memcpy((char *)buff, &Command_46_2B[0], 8);
					break;
				case 0x002D:
					memcpy((char *)buff, &Command_46_2D[0], 8);
					break;
				case 0x002E:
					memcpy((char *)buff, &Command_46_2E[0], 8);
					break;
				case 0x002F:
					memcpy((char *)buff, &Command_46_2F[0], 8);
					break;
				case 0x003B:
					memcpy((char *)buff, &Command_46_3B[0], 8);
					break;
				default:
					mscdInfo.SenseKey = 0x05;  //INVALID COMMAND
					mscdInfo.ASC = 0x24;
					mscdInfo.ASCQ = 0;
			}
		}
	}
	else if (CBW_In.CBWD.LUN == 0x01)
	{
		if (len > 176)
			len = 176;
		memcpy((char *)buff, &Command_46_01[0], len);
	}
	else if (CBW_In.CBWD.LUN == 0x00)
	{
		if (len > 328)
			len = 328;

		memset(buff, 0x0, len);
		if (get_be16(&CBW_In.CBWD.LBA.d[0]) == 0x0000)
			memcpy((char *)buff, &Command_46_00[0], len);
		else
		{
			memcpy((char *)buff, &Command_46_00[0], 8);
			buff += 8;
			switch(get_be16(&CBW_In.CBWD.LBA.d[0]))
			{
				case 0x001E:
					memcpy((char *)buff, &Command_46_1E[0], 8);
					break;
				case 0x001F:
					memcpy((char *)buff, &Command_46_1F[0], 8);
					break;
				case 0x0020:
					memcpy((char *)buff, &Command_46_20[0], 8);
					break;
				case 0x0021:
					memcpy((char *)buff, &Command_46_21[0], 8);
					break;
				case 0x0023:
					memcpy((char *)buff, &Command_46_23[0], 4);
					break;
				case 0x0024:
					memcpy((char *)buff, &Command_46_24[0], 8);
					break;
				case 0x0026:
					memcpy((char *)buff, &Command_46_26[0], 4);
					break;
				case 0x002A:
					memcpy((char *)buff, &Command_46_2A[0], 8);
					break;
				case 0x002B:
					memcpy((char *)buff, &Command_46_2B[0], 8);
					break;
				case 0x002D:
					memcpy((char *)buff, &Command_46_2D[0], 8);
					break;
				case 0x002E:
					memcpy((char *)buff, &Command_46_2E[0], 8);
					break;
				case 0x002F:
					memcpy((char *)buff, &Command_46_2F[0], 8);
					break;
				case 0x003B:
					memcpy((char *)buff, &Command_46_3B[0], 8);
					break;
				default:
					mscdInfo.SenseKey = 0x05;  //INVALID COMMAND
					mscdInfo.ASC = 0x24;
					mscdInfo.ASCQ = 0;
			}
		}
	}
	else
	{
		mscdInfo.SenseKey = 0x05;  //INVALID COMMAND
		mscdInfo.ASC = 0x24;
		mscdInfo.ASCQ = 0;
	}
	mscdSDRAM2USB_Bulk(mscdInfo.Storage_Base_Addr ,len);	
}

static UINT8 Command_43_00_01[12] = {
	0x00, 0x0A, 0x01, 0x01, 0x00, 0x14, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00 };

static UINT8 Command_43_00_00[20] = {
	0x00, 0x12, 0x01, 0x01, 0x00, 0x14, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xAA, 0x00,
	0x00, 0x00, 0x01, 0x2E };

static UINT8 Command_43_02[20] = {
	0x00, 0x12, 0x01, 0x01, 0x00, 0x14, 0x01, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x14, 0xAA, 0x00,
	0x00, 0x00, 0x06, 0x02 };


VOID mscdCommand_43(void)
{
	UINT8 j;
	UINT32 tmpval=mscdInfo.Mass_Base_Addr;
	if (CBW_In.CBWD.LUN == 0x02)
	{
		if (CBW_In.CBWD.dCBWDataTferLh.c32 > 20)
		{
			for (j = 0; j<20; j++)
				writeb(tmpval+j, Command_43_02[j]);
			mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr ,20);
		}
		else
		{
			for (j = 0; j<CBW_In.CBWD.dCBWDataTferLh.c32; j++)
				writeb(tmpval+j,Command_43_02[j]);
			mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , CBW_In.CBWD.dCBWDataTferLh.c32);
		}
	}
	else if (CBW_In.CBWD.LUN == 0x00)
	{
		switch (CBW_In.CBWD.LBA.d[0])
		{
			case 0x01:
				if (CBW_In.CBWD.dCBWDataTferLh.c32 > 12)
				{
					for (j = 0; j<12; j++)
						writeb(tmpval+j, Command_43_00_01[j]);
					mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , 12);
				}
				else
				{
					for (j = 0; j<CBW_In.CBWD.dCBWDataTferLh.c32; j++)
						writeb(tmpval+j, Command_43_00_01[j]);
					mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , CBW_In.CBWD.dCBWDataTferLh.c32);
				}
				break;

			case 0x00:
				if (CBW_In.CBWD.dCBWDataTferLh.c32 > 20)
				{
					for (j = 0; j<20; j++)
						writeb(tmpval+j, Command_43_00_00[j]);
					mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , 20);
				}
				else
				{
					for (j = 0; j<CBW_In.CBWD.dCBWDataTferLh.c32; j++)
						writeb(tmpval+j, Command_43_00_00[j]);
					mscdSDRAM2USB_Bulk(mscdInfo.Mass_Base_Addr , CBW_In.CBWD.dCBWDataTferLh.c32);
				}
				break;

			default:
				mscdInfo.SenseKey = 0x05;  //INVALID COMMAND
				mscdInfo.ASC = 0x24;
				mscdInfo.ASCQ = 0;
		}
	}
	else
	{
		mscdInfo.SenseKey = 0x05;  //INVALID COMMAND
		mscdInfo.ASC = 0x24;
		mscdInfo.ASCQ = 0;
	}
}


UINT8 ModeSense_2A[40] = {
	0x00, 0x36, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2A, 0x2E, 0x3F, 0x37, 0xF5, 0x73, 0x29, 0x23,
	0x1B, 0x90, 0x01, 0x00, 0x05, 0x8B, 0x1B, 0x90,
	0x00, 0x00, 0x1B, 0x90, 0x1B, 0x90, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x01, 0x1B, 0x90, 0x00, 0x04
};

UINT8 ModeSense_1A[12] = {
	0x00, 0x12, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1A, 0x0A, 0x00, 0x03
};

void mscdModeSense_Command_CDROM(void)
{
	UINT8 len;
	len = CBW_In.CBWD.dCBWDataTferLh.c32;							
	            
	memset((char *)mscdInfo.Storage_Base_Addr, 0x0, len);
	
	switch (CBW_In.CBWD.LBA.d[0])
	{
		case 0x2A:
			memcpy((char *)mscdInfo.Storage_Base_Addr, &ModeSense_2A[0], 40);
			break;
		case 0x1A:
			memcpy((char *)mscdInfo.Storage_Base_Addr, &ModeSense_1A[0], 12);
			break;
		default:
			mscdInfo.SenseKey = 0x05;  //INVALID COMMAND
			mscdInfo.ASC = 0x24;
			mscdInfo.ASCQ = 0;
	}	     


	mscdSDRAM2USB_Bulk(mscdInfo.Storage_Base_Addr, len);
}
#endif

VOID mscdBlcokModeEnable(BOOL bEnable)
{
	if(bEnable)
		g_mscd_block_mode = 1;
	else
		g_mscd_block_mode = 0;
}
