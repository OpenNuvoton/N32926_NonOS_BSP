/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include "wblib.h"
#include "string.h"
#include "wbio.h"
#include "w55fa92_reg.h"
#include "DrvAAC.h"


//#define OPT_FPGA_DEBUG
#define OPT_FA95_SPU
#define	E_SUCCESS	0

//#define __INT__ 


#ifdef __INT__
volatile static INT32 g_i32flag;

void DrvMDCT_IntHandler(void)
{
    if ( g_i32flag == 0x1 ) 
    {
	    if ( inp32(REG_MDCTINT) & DMAIN_INT )
    	{
        	outp32(REG_MDCTINT, DMAIN_INT);
   	    	g_i32flag = 0;
//   	    	sysprintf("INT 0x1");
	    }
    }
    if ( g_i32flag == 0x2 )
    {
      if ( inp32(REG_MDCTINT) & MDCT_INT )
      {
      	  outp32(REG_MDCTINT, MDCT_INT);
      	  while (1)
      	  {
      	  	 if ((inp32(REG_MDCTSTATE) & 0x0F) == 0 )
      	  	 {
      	  	 	break;
      	  	 }
      	  }	
      	  g_i32flag = 0;
//    	  sysprintf("INT 0x2");
      }
    }
    if ( g_i32flag == 0x04 )
    {
      if ( inp32(REG_MDCTINT) & DMAOUT_INT )
      {
          outp32(REG_MDCTINT, DMAOUT_INT);
          g_i32flag = 0;
//   	    	sysprintf("INT 0x4");          
      }
    }
    
}
#endif


// AAC open
ERRCODE
DrvAAC_Open(void)
{

    // enable AHB4 clock
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) | HCLK4_CKE);
       
	// enable AAC engine clock 
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) | AAC_CKE);			// enable AAC engine clock 
  
	// reset AAC engine 
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) | AAC_RST);
	outp32(REG_AHBIPRST, inp32(REG_AHBIPRST) & ~AAC_RST);	
#ifdef __INT__	
	sysInstallISR(IRQ_LEVEL_1, IRQ_MDCT, (PVOID)DrvMDCT_IntHandler);	
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_MDCT);
#endif	
	

	return E_SUCCESS;
}

// AAC close
void DrvAAC_Close(void)
{
#ifdef __INT__   
   	sysDisableInterrupt(IRQ_MDCT);
#endif   	
      // disable AHB4  clock
    outp32(REG_AHBCLK, inp32(REG_AHBCLK) & (~HCLK4_CKE));
 	// disable AAC engine clock 
	outp32(REG_AHBCLK2, inp32(REG_AHBCLK2) & (~AAC_CKE));			// disable AAC engine clock 
    // ignore AHB4 & AHB clock disabled, maybe the other uses it
}

INT32
DrvAAC_Decoder(
	INT32  i32Size,
	INT32 *pi32inbuf,
	INT32 *pi32outbuf
)
{

   INT32 i32OutputSize, i32InputWord, i32OutputWord;
   i32Size >>= 1;    // n/2 = 1024 or 128
// DMA read
    i32InputWord = i32Size  - 1;
	outp32(REG_DMA_RADDR, (UINT32)pi32inbuf);  // set DMA read buffer
	outp32(REG_DMA_LENGTH, i32InputWord);   // Set DMA length 
	outp32(REG_DMA_DIRECTION, 0);   // 0 for read data in
#ifdef __INT__	
	outp32(REG_MDCTINT, DMAIN_INT_ENABLE);  // enable DMA IN interrupt
#endif	
    outp32(REG_DMA_STATE, DMA_STATE);        //	DMA enable or idle, poll??
    
#ifdef __INT__
    g_i32flag = 0x1;
    while (g_i32flag == 0x1); 
#else    
    while (1)
    {
      if ( inp32(REG_MDCTINT) & DMAIN_INT )
      {
          outp32(REG_MDCTINT, DMAIN_INT);
          break;
      }
    }
#endif           
	
// MDCT decoder
   if (i32Size == 1024 )
   {
	   outp32(REG_MDCTPAR, WIN_2048|DECODEREN);
	   i32OutputSize = 2048;
   }
   else
   {
       outp32(REG_MDCTPAR, WIN_256|DECODEREN);
       i32OutputSize = 256;
   }
   
   i32OutputWord = i32OutputSize - 1;  
#ifdef __INT__    
   outp32(REG_MDCTINT, MDCT_INT_ENABLE);
#endif   
   outp32(REG_MDCTCTL, MDCTEN);
   
#ifdef __INT__
    g_i32flag = 0x2;
    while (g_i32flag == 0x2); 
#else    
   while (1)
   {
      if ( inp32(REG_MDCTINT) & MDCT_INT )
      {
      	  outp32(REG_MDCTINT, MDCT_INT);
      	  while (1)
      	  {
      	  	 if ((inp32(REG_MDCTSTATE) & 0x0F) == 0 )
      	  	 {
      	  	 	break;
      	  	 }
      	  }	
      	  break;
      }
   }
#endif   
   
// DMA write
	outp32(REG_DMA_WADDR, (UINT32)pi32outbuf);  // set DMA write buffer
	outp32(REG_DMA_LENGTH, i32OutputWord);   // Set DMA length 
	outp32(REG_DMA_DIRECTION, 1);   // 1 for write data out, 0 for data in
#ifdef __INT__	
	outp32(REG_MDCTINT, DMAOUT_INT_ENABLE);  // enable DMA write interrupt
#endif	
    outp32(REG_DMA_STATE, DMA_STATE);        //	DMA enable or idle, poll??
    
#ifdef __INT__
    g_i32flag = 0x4;
    while (g_i32flag == 0x4); 
#else    
 
    while (1)
    {
      if ( inp32(REG_MDCTINT) & DMAOUT_INT )
      {
          outp32(REG_MDCTINT, DMAOUT_INT);
          break;
      }
    }       
#endif   	
    return i32OutputSize;    	   	
}



INT32
DrvAAC_Encoder(
	INT32 *pi32inbuf, 
	INT32 *pi32outbuf,
	INT32  i32Size
	
)
{

   INT32 i32OutputSize, i32InputWord, i32OutputWord;
// DMA output
    i32InputWord = i32Size - 1;
	outp32(REG_DMA_RADDR, (UINT32)pi32inbuf);  // set DMA write buffer
	outp32(REG_DMA_LENGTH, i32InputWord);   // Set DMA length 
	outp32(REG_DMA_DIRECTION, 0);   // 0 for read data in
#ifdef __INT__	
	outp32(REG_MDCTINT, DMAIN_INT_ENABLE);  // enable DMA IN interrupt
#endif	
    outp32(REG_DMA_STATE, DMA_STATE);        //	DMA enable or idle, poll??
#ifdef __INT__
    g_i32flag = 0x1;
    while (g_i32flag == 0x1); 
#else        
    while (1)
    {
      if ( inp32(REG_MDCTINT) & DMAIN_INT )
      {
          outp32(REG_MDCTINT, DMAIN_INT);
          break;
      }
    }       
#endif
	
// MDCT encoder
   if (i32Size == 2048 )
   {
	   outp32(REG_MDCTPAR, WIN_2048); //|(~DECODEREN));
	   i32OutputSize = 1024;
   }
   else
   {
       outp32(REG_MDCTPAR, WIN_256); //|(~DECODEREN));
       i32OutputSize = 128;
   }
   i32OutputWord = i32OutputSize - 1;  
#ifdef __INT__   
   outp32(REG_MDCTINT, MDCT_INT_ENABLE);
#endif   
   outp32(REG_MDCTCTL, MDCTEN);
#ifdef __INT__
    g_i32flag = 0x2;
    while (g_i32flag == 0x2); 
#else       
   while (1)
   {
      if ( inp32(REG_MDCTINT) & MDCT_INT )
      {
      	  outp32(REG_MDCTINT, MDCT_INT);
      	  while (1)
      	  {
      	  	 if ((inp32(REG_MDCTSTATE) & 0x0F) == 0)
      	  	 {
      	  	 	break;
      	  	 }
      	  }	
      	  break;
      }
   }
#endif   
// DMA output
	outp32(REG_DMA_WADDR, (UINT32)pi32outbuf);  // set DMA write buffer
	outp32(REG_DMA_LENGTH, i32OutputWord);   // Set DMA length 
	outp32(REG_DMA_DIRECTION, 1);   // 1 for write data out, 1 for data in
#ifdef __INT__	
    outp32(REG_MDCTINT, DMAOUT_INT_ENABLE);  // enable DMA OUT interrupt
#endif	
    outp32(REG_DMA_STATE, DMA_STATE);        //	DMA enable or idle, poll??
#ifdef __INT__
    g_i32flag = 0x4;
    while (g_i32flag == 0x4); 
#else        
    while (1)
    {
      if ( inp32(REG_MDCTINT) & DMAOUT_INT )
      {
          outp32(REG_MDCTINT, DMAOUT_INT);
          break;
      }
    }       
#endif   	
    return i32OutputSize;    	   	
}


