/**************************************************************************//**
 * @file     Demo_AAC.c
 * @brief    Demo AAC IMDCT/MDCT for the engine of decoder and encoder
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma import(__use_no_semihosting_swi)
#include "wbio.h"
#include "wblib.h"
#include "DrvAAC.h"


// decoder input test pattern
#include "Dec_Input_128.h"
#include "Dec_Input_1024.h"

// decoder output result
#include "Dec_Output_256.h"
#include "Dec_Output_2048.h"

// encoder input test pattern
#include "Enc_Input_256.h"
#include "Enc_Input_2048.h"

//encoder output result
#include "Enc_Output_128.h"
#include "Enc_Output_1024.h"


//#pragma import(__use_no_semihosting_swi)

INT32  ai32outbuf[2048];

#define DECODER_THRESHOLD	256
#define NON_CACHE_ADDRESS	0x80000000

#define TESTNO				50
	
int main()
{
//    WB_UART_T 	uart;
	INT32  error; 
	INT32  *pi32inptr, *pi32outptr, *pi32testptr, *pi32resultptr;
//	UINT32 u32HClk1Clock, u32HClk4Clock;
//	UINT32 u32HClk4Div;

	INT32  i, j, kk;
    UINT32 u32ExtFreq;
    WB_UART_T uart;

    //--- initial UART
    u32ExtFreq = sysGetExternalClock();
	uart.uart_no = WB_UART_1; 
	uart.uiFreq = u32ExtFreq;	//use APB clock
   	uart.uiBaudrate = 115200;
   	uart.uiDataBits = WB_DATA_BITS_8;
   	uart.uiStopBits = WB_STOP_BITS_1;
   	uart.uiParity = WB_PARITY_NONE;
   	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
   	sysInitializeUART(&uart);
 	/**********************************************************************************************
     * Clock Constraints: 
     * (a) If Memory Clock > System Clock, the source clock of Memory and System can come from
     *     different clock source. Suggestion MPLL for Memory Clock, UPLL for System Clock   
     * (b) For Memory Clock = System Clock, the source clock of Memory and System must come from 
     *     same clock source	 
     *********************************************************************************************/

#if 0 
    /********************************************************************************************** 
     * Slower down system and memory clock procedures:
     * If current working clock fast than desired working clock, Please follow the procedure below  
     * 1. Change System Clock first
     * 2. Then change Memory Clock
     * 
     * Following example shows the Memory Clock = System Clock case. User can specify different 
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement. 
     *********************************************************************************************/
    sysSetSystemClock(eSYS_EXT, 12000000, 12000000);
    sysSetDramClock(eSYS_EXT, 12000000, 12000000);
#else 
    /********************************************************************************************** 
     * Speed up system and memory clock procedures:
     * If current working clock slower than desired working clock, Please follow the procedure below  
     * 1. Change Memory Clock first
     * 2. Then change System Clock
     * 
     * Following example shows to speed up clock case. User can specify different 
     * Memory Clock and System Clock depends on DRAM bandwidth or power consumption requirement.
     *********************************************************************************************/
    sysSetDramClock(eSYS_MPLL, 360000000, 360000000);
    sysSetSystemClock(eSYS_UPLL,            //E_SYS_SRC_CLK eSrcClk,
                      240000000,            //UINT32 u32PllKHz,
                      240000000);           //UINT32 u32SysKHz,
    sysSetCPUClock(240000000/2);
		sysprintf("CPU Clock = %d\n", sysGetCPUClock());
#endif   
    sysprintf("\n=====> W55FA92 Emulation for AAC =====\n");	

	
//   	outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) & 0xFFFFFF0F) | 0x00);	
//   	outp32(REG_CLKDIV4, (inp32(REG_CLKDIV4) & 0xFFFFFF0F)  | 0x80);		// 0x00 ~ 0xF0
	
	/* CACHE_ON	*/
	sysEnableCache(CACHE_WRITE_BACK);


    DrvAAC_Open();
    
    

    kk = 0; 
    while (kk < 100)
    { 
       kk++;
// stage 4
	sysprintf("Encoder Test, input non cache address, output non cache address\n");
	sysprintf("Windows 256 testing\n");

	    
	pi32inptr = Enc_Input_256 ;
	pi32outptr = Enc_Output_128;	
	
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);

	error = 0;
	for (i=0; i< TESTNO ; i++)
	{

	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS);		
		DrvAAC_Encoder(pi32inptr,pi32resultptr,256);
		
		pi32testptr = pi32outptr;
		
	
		for (j=0; j<128; j++)
		{
			if ( (*pi32testptr) != (*pi32resultptr) )
			{
//				sysprintf("Pattern dismatch %d files,%d value\n",i, j);
//				while (1);
				error++;
			}
			pi32testptr++;
			pi32resultptr++;

		}
	    pi32inptr += 256;
	    pi32outptr += 128;
    }	
    if ( error == 0 )
    {
     	sysprintf("Encoder for Windows 256 testing is OK\n");
   	}
   	else
   	{
     	sysprintf("Encoder for Windows 256 testing failed\n"); 
     	while (1);
 	}
		
		
    sysprintf("Windows 2048 testing\n");
	
	pi32inptr = Enc_Input_2048 ;
	pi32outptr = Enc_Output_1024;	
	
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);
	
	error = 0;
	for (i=0; i< TESTNO; i++)
	{
	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS);
		DrvAAC_Encoder(pi32inptr,pi32resultptr,2048);
		pi32testptr = pi32outptr;
		for (j=0; j<1024; j++)
		{
			if ( (*pi32testptr) != (*pi32resultptr) )
			{
//				sysprintf("Pattern dismatch %d files,%d value\n",i, j);
//				while (1);
				error++;
			}
			pi32testptr++;
			pi32resultptr++;

		}
	    pi32inptr += 2048;
	    pi32outptr += 1024;
    }	
    if ( error == 0 )
    {
     	sysprintf("Encoder for Windows 2048 testing is OK\n");
   	}		
   	else
   	{
     	sysprintf("Encoder for Windows 2048 testing failed\n");
     	while (1);     	 
 	}
	
    
    sysprintf("Windows 256 testing\n");

	    
	pi32inptr = Enc_Input_256;
	pi32outptr = Enc_Output_128;	

	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);
	
	error = 0;
	for (i=0; i< TESTNO ; i++)
	{

	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS);		
		DrvAAC_Encoder(pi32inptr,pi32resultptr,256);
		
		pi32testptr = pi32outptr;
		
	
		for (j=0; j<128; j++)
		{
			if ( (*pi32testptr) != (*pi32resultptr) )
			{
//				sysprintf("Pattern dismatch %d files,%d value\n",i, j);
//				while (1);
				error++;
			}
			pi32testptr++;
			pi32resultptr++;

		}
	    pi32inptr += 256;
	    pi32outptr += 128;
    }	
    if ( error == 0 )
    {
     	sysprintf("Encoder for Windows 256 testing is OK\n");
   	}
   	else
   	{
     	sysprintf("Encoder for Windows 256 testing failed\n"); 
     	while (1);     	
 	}
   	
		

	sysprintf("Windows 2048 testing\n");
	
	pi32inptr = Enc_Input_2048 ;
	pi32outptr = Enc_Output_1024;	
	
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);
		
	error = 0;
	for (i=0; i< TESTNO ; i++)
	{
	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS);
		DrvAAC_Encoder(pi32inptr,pi32resultptr,2048);
		pi32testptr = pi32outptr;
		for (j=0; j<1024; j++)
		{
			if ( (*pi32testptr) != (*pi32resultptr) )
			{
//				sysprintf("Pattern dismatch %d files,%d value\n",i, j);
//				while (1);
				error++;
			}			
			pi32testptr++;
			pi32resultptr++;

		}
	    pi32inptr += 2048;
	    pi32outptr += 1024;
    }	
    if ( error == 0 )
    {
     	sysprintf("Encoder for Windows 2048 testing is OK\n");
   	}			
   	else
   	{
     	sysprintf("Encoder for Windows 2048 testing failed\n"); 
     	while (1);     	
 	}


	sysprintf("Decoder Test, input non cache address, output non cache address\n");
	
	sysprintf("Windows 256 testing\n");

	pi32inptr =  Dec_Input_128;
	pi32outptr = Dec_Output_256;	
	
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);
	
	error = 0;
	for (i=0; i< TESTNO ; i++)
	{
	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS);	
		DrvAAC_Decoder(128*2, pi32inptr,pi32resultptr);
		pi32testptr = pi32outptr;
		for (j=0; j< 256; j++)
		{
            if ( (*pi32testptr) != (*pi32resultptr) )
            {
				error++;			    
			}    
			pi32testptr++;
			pi32resultptr++;

		}
	    pi32inptr += 128;
	    pi32outptr += 256;
    }	
    if ( error == 0 )
    {
     	sysprintf("Decoder for Windows 256 testing is OK\n");
   	}	
   	else
   	{
     	sysprintf("Decoder for Windows 256 testing failed\n");
     	while (1);     	
   	}	

	sysprintf("Windows 2048 testing\n");
	
	pi32inptr = Dec_Input_1024;
	pi32outptr = Dec_Output_2048;
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);	
	error = 0;
	for (i=0; i< TESTNO; i++)
	{
	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS );
		DrvAAC_Decoder(1024*2,pi32inptr,pi32resultptr);
		pi32testptr = pi32outptr;
		for (j=0; j< 2048; j++)
		{
            if ( (*pi32testptr) != (*pi32resultptr) )
            {
				error++;
			}
			pi32testptr++;
			pi32resultptr++;
		}
	    pi32inptr += 1024;
	    pi32outptr += 2048;
    }	
    if ( error == 0 )
    {
     	sysprintf("Decoder for Windows 2048 testing is OK\n");
   	}		
   	else
   	{
     	sysprintf("Decoder for Windows 2048 testing failed\n");
     	while (1);     	
   		
   	}

	sysprintf("Windows 256 testing\n");

	pi32inptr =  Dec_Input_128;
	pi32outptr = Dec_Output_256;	
	
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);
	
	error = 0;
	for (i=0; i< TESTNO ; i++)
	{
	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS);	
		DrvAAC_Decoder(128*2, pi32inptr,pi32resultptr);
		pi32testptr = pi32outptr;
		for (j=0; j< 256; j++)
		{
            if ( (*pi32testptr) != (*pi32resultptr) )
            {
				error++;			    
			}    
			pi32testptr++;
			pi32resultptr++;

		}
	    pi32inptr += 128;
	    pi32outptr += 256;
    }	
    if ( error == 0 )
    {
     	sysprintf("Decoder for Windows 256 testing is OK\n");
   	}	
   	else
   	{
     	sysprintf("Decoder for Windows 256 testing failed\n");
     	while (1);     	
   	}	

	sysprintf("Windows 2048 testing\n");
	
	pi32inptr = Dec_Input_1024;
	pi32outptr = Dec_Output_2048;
	pi32inptr = (INT32 *)((INT32)pi32inptr | NON_CACHE_ADDRESS);	
	error = 0;
	for (i=0; i< TESTNO ; i++)
	{
	    pi32resultptr = (INT32 *)((INT32)ai32outbuf | NON_CACHE_ADDRESS );
		DrvAAC_Decoder(1024*2, pi32inptr,pi32resultptr);
		pi32testptr = pi32outptr;
		for (j=0; j< 2048; j++)
		{
            if ( (*pi32testptr) != (*pi32resultptr) )
            {
				error++;
			}
			pi32testptr++;
			pi32resultptr++;
		}
	    pi32inptr += 1024;
	    pi32outptr += 2048;
    }	
    if ( error == 0 )
    {
     	sysprintf("Decoder for Windows 2048 testing is OK\n");
   	}		
   	else
   	{
     	sysprintf("Decoder for Windows 2048 testing failed\n");
     	while (1);     	
   		
   	}
   	}    
    DrvAAC_Close();   
}



