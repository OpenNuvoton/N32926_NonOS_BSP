//==================================================================
//                                                                          *
// Copyright (c) 2006 Winbond Electronics Corp. All rights reserved.        *
//                                                                          *
//==================================================================
 
//===================================================================
// 
// FILENAME
//     Rotation.c
//
// VERSION
//     0.1
//
// DESCRIPTION
//     Low Level I/O for Rotation Engine for W55FA92
//
// DATA STRUCTURES
//     None
//
// FUNCTIONS
//     None
//
// HISTORY
//     12/08/30		
//                   
//
// REMARK
//     None
//===================================================================
#include "W55FA92_reg.h"
#include "wblib.h"
#include "rotlib.h"

PFN_ROT (RotIntHandlerTable)[3]={0};

INT32 rotSrcLineOffset(UINT32 uSrcFmt, UINT uSrcPacLineOffset);
INT32 rotDstLineOffset(UINT32 uSrcFmt, UINT uDstPacLineOffset);
void rotPacketResetDstBufferAddr(T_ROT_CONF* ptRotConf);
void CALLBACK rotIntHandler(void);
void rotInit(BOOL bIsRotEngMode, BOOL bIsRotIntEnable);

//#define DBG_PRINTF       sysprintf
#define DBG_PRINTF(...) 		
//Install the call back function for Rotation
void rotInstallISR(UINT32 u32IntNum,PVOID pvIsr)
{
   	RotIntHandlerTable[u32IntNum] = (PFN_ROT)(pvIsr);
}

void  rotIntHandler(void)
{
	UINT32 u32IntStatus =  inp32(REG_RICR);
        if (u32IntStatus & ROTE_FINISH)
	{//Complete
		if(RotIntHandlerTable[0]!=0)			     
		        RotIntHandlerTable[0]();		
		outp32(REG_RICR, inp32(REG_RICR) & 0xFFFF0000 | ROTE_FINISH);		//Clear interrupt Complete status
	}	
	else if( u32IntStatus & TG_ABORT )
	{//Abnormal 
		if(RotIntHandlerTable[1]!=0)
                        RotIntHandlerTable[1]();
		outp32(REG_RICR, inp32(REG_RICR) & 0xFFFF0000 | TG_ABORT);		//Clear Target Abort
	}
	else if(u32IntStatus & SRAM_OF )
	{//Abnormal 
		if(RotIntHandlerTable[2]!=0)	
                        RotIntHandlerTable[2]();

		outp32(REG_RICR, inp32(REG_RICR) & 0xFFFF0000 | SRAM_OF);		//Clear Overflow	
	}
}

//===================================================================
//	
//							
//						
//
//			
//
//
//
//
//===================================================================
void rotOpen(void)
{
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | ROTE_CKE);	
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | SRAM_CKE);
		
	outp32(REG_CHIPCFG, inp32(REG_CHIPCFG) & ~ROT_IRM);	//32K RAM for ROT
	
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | ROT_RST);			
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~ROT_RST);	
	outp32(REG_SCCR, inp32(REG_SCCR) & ~ROTSW_RST);		/* reset ROT */
	outp32(REG_SCCR, inp32(REG_SCCR) | ROTSW_RST);			
	outp32(REG_SCCR, inp32(REG_SCCR) & ~ROTSW_RST);		/* reset ROT */
	sysInstallISR(IRQ_LEVEL_1, IRQ_ROT, (PVOID)rotIntHandler);	
	sysEnableInterrupt(IRQ_ROT);
	outp32(REG_RICR,inp32(REG_RICR) | (SRAM_OF_EN | TG_ABORT_EN | ROTE_INT_EN));	
}

void rotClose(void)
{
	sysDisableInterrupt(IRQ_ROT);
	outp32(REG_RICR,inp32(REG_RICR) | (SRAM_OF_EN | TG_ABORT_EN | ROTE_INT_EN));
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & ~ROTE_CKE);	
	outp32(REG_CHIPCFG, inp32(REG_CHIPCFG) | ROT_IRM);	//32K RAM for CPU
}

INT32 rotImageConfig(T_ROT_CONF* ptRotConf)
{
	UINT32 uRotCfg=0;
	uRotCfg = uRotCfg | (ptRotConf->eRotFormat<<6);	
	uRotCfg = uRotCfg | (ptRotConf->eBufSize<<4);
	uRotCfg = uRotCfg |	(ptRotConf->eRotDir<<1);	
	outp32(REG_RCR, uRotCfg);
	outp32(REG_RIS, ptRotConf->u32RotDimHW);

	rotSrcLineOffset( ptRotConf->eRotFormat, ptRotConf->u32SrcLineOffset);
	outp32(REG_RSISA, ptRotConf->u32SrcAddr);
	rotDstLineOffset( ptRotConf->eRotFormat, ptRotConf->u32DstLineOffset);
	rotPacketResetDstBufferAddr(ptRotConf);	
	return Successful;
}

INT32 rotSrcLineOffset(UINT32 uSrcFmt, UINT32 u32SrcPacLineOffset)
{
	switch(uSrcFmt)
	{
    		case E_ROT_PACKET_RGB565:		
    		case E_ROT_PACKET_YUV422:			
    			u32SrcPacLineOffset = u32SrcPacLineOffset*2;			
    			break;
    		case E_ROT_PACKET_RGB888:		
    			u32SrcPacLineOffset = u32SrcPacLineOffset*4;			
    			break;
	}
	outp32(REG_RSILOFF, u32SrcPacLineOffset);						
	return Successful;
}
INT32 rotDstLineOffset(UINT32 u32SrcFmt, UINT32 u32DstPacLineOffset)
{
	switch(u32SrcFmt)
	{	
    		case E_ROT_PACKET_RGB565:
    		case E_ROT_PACKET_YUV422:			
    			u32DstPacLineOffset = u32DstPacLineOffset*2;			
    			break;
    		case E_ROT_PACKET_RGB888:		
    			u32DstPacLineOffset = u32DstPacLineOffset*4;			
    			break;
	}
	outp32(REG_RDILOFF, u32DstPacLineOffset);						
	return Successful;
}


INT32 rotTrigger(void)
{
	INT32 i32ErrCode = Successful;
	
	if(inp32(REG_RCR) & ROTE_EN)
	{//Busy 
		i32ErrCode = (INT32)ERR_ROT_BUSY;	
	}
	else
	{//Ready
		outp32(REG_RCR, inp32(REG_RCR) | ROTE_EN);	
	}	
	return i32ErrCode;
}
INT32 rotGetPacketPixelWidth(E_ROTENG_FMT ePacFormat)
{
	INT32 i32PixelWidth = 2;
	switch(ePacFormat)
	{
		case E_ROT_PACKET_RGB565:		i32PixelWidth = 2;	break;					 
		case E_ROT_PACKET_RGB888:		i32PixelWidth = 4;	break;	
		case E_ROT_PACKET_YUV422:		i32PixelWidth = 2;	break;	
	}	
	return i32PixelWidth;
}
VOID rotPacketResetDstBufferAddr(T_ROT_CONF* ptRotConf)
{
	UINT32 u32OrigPacketRealImagWidth=0, u32OrigPacketRealImagHeight=0, u32RotPacketRealImagWidth=0;
	UINT32 u32Poffset=0, u32StartAdd0;
	UINT8 u8PixelWidth;
	UINT32 u32LineOffset = 0;
    
	u32StartAdd0 = ptRotConf->u32DstAddr;
	u32OrigPacketRealImagHeight = (ptRotConf->u32RotDimHW)>>16;
	u32OrigPacketRealImagWidth =  (ptRotConf->u32RotDimHW)&0x0FFFF;
	//Consider Offset				
	
	u32LineOffset = ptRotConf->u32DstLineOffset;                          //Pixel unit
	u8PixelWidth=rotGetPacketPixelWidth(ptRotConf->eRotFormat);

    	//Consider rotation
	switch(ptRotConf->eRotDir)
	{	
		case E_ROT_ROT_L90:			
		case E_ROT_ROT_R90:
			u32RotPacketRealImagWidth=u32OrigPacketRealImagHeight+u32LineOffset;
			//u32RotPacketRealImagHeight=u32OrigPacketRealImagWidth;		
			break;	
		default:
			u32RotPacketRealImagWidth=u32OrigPacketRealImagWidth+u32LineOffset;
			//u32RotPacketRealImagHeight=u32OrigPacketRealImagHeight;		
			break;	
	}			
	switch(ptRotConf->eRotDir)
	{
		case E_ROT_ROT_L90:
		u32Poffset=(u32RotPacketRealImagWidth)*(u32OrigPacketRealImagWidth-1)*u8PixelWidth;						
		break;
		case E_ROT_ROT_R90:
		u32Poffset=(u32OrigPacketRealImagHeight-1)*u8PixelWidth; 		
		break;		
	}	
        u32StartAdd0 = u32StartAdd0 +u32Poffset;		
        DBG_PRINTF("Rot_Dst_Address0= 0x%x\n", u32StartAdd0);
        DBG_PRINTF("Rot_Dst_Address1= 0x%x\n", u32StartAddr1);
        DBG_PRINTF("\n");
	outp32(REG_RDISA, u32StartAdd0);
}
