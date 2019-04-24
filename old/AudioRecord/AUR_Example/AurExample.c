/***************************************************************************
 *                                                                         									     *
 * Copyright (c) 2008 Nuvoton Technolog. All rights reserved.              					     *
 *                                                                         									     *
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "nvtfat.h"
#include "W55FA92_SIC.h"
#include "spu.h"
#include "AurExample.h"


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card insert
 *---------------------------------------------------------------------------*/
void isr_card_insert()
{
    UINT32 result;
    sysprintf("--- ISR: card inserted on SD port 0 ---\n\n");
    result = sicSdOpen0();
    if (result < FMI_ERR_ID)
    {
        sysprintf("    Detect card on port %d.\n", 0);
    }
    else if (result == FMI_NO_SD_CARD)
    {
        sysprintf("WARNING: Don't detect card on port %d !\n", 0);
    }
    else
    {
        sysprintf("WARNING: Fail to initial SD/MMC card %d, result = 0x%x !\n", 0, result);
    }
    return;
}


/*-----------------------------------------------------------------------------
 * ISR of Card detect interrupt for card remove
 *---------------------------------------------------------------------------*/
void isr_card_remove()
{
    sysprintf("--- ISR: card removed on SD port 0 ---\n\n");
    sicSdClose0();
    return;
}

void init(void)
{
	WB_UART_T uart;
	UINT32 u32ExtFreq, u32PllOutHz;
	char ucInput;
	
	u32ExtFreq = sysGetExternalClock();
	uart.uiFreq = u32ExtFreq;//27000000;//;	//use APB clock
    	uart.uiBaudrate = 115200;
    	uart.uiDataBits = WB_DATA_BITS_8;
    	uart.uiStopBits = WB_STOP_BITS_1;
    	uart.uiParity = WB_PARITY_NONE;
    	uart.uiRxTriggerLevel = LEVEL_1_BYTE;
    	uart.uart_no = WB_UART_1;
    	sysInitializeUART(&uart);
    	
    	u32ExtFreq = sysGetExternalClock();
	u32PllOutHz = sysGetPLLOutputHz(eSYS_UPLL, u32ExtFreq);
    	sysSetTimerReferenceClock (TIMER0, u32ExtFreq);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);
	
    	/* Init file system */
	DBG_PRINTF(" Do you want to initialize SD card 0 (y?).\n");
	//ucInput = sysGetChar();
	ucInput = 'y';
	if((ucInput=='y') || (ucInput=='Y'))
	{	
		/* Init file system */
		

		fsInitFileSystem();
		/* Init Storage Interface Controller */
		sicIoctl(SIC_SET_CLOCK, u32PllOutHz/1000, 0, 0);	
		sicOpen();	
		 //--- Initial callback function for card detection interrupt
		sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);
		if (sicSdOpen0()<=0)
		{
			sysprintf("Error in initializing SD card !! \n");						
			while(1);
		}			
		fsAssignDriveNumber('C', DISK_TYPE_SD_MMC, 0, 1);
		sicIoctl(SIC_SET_CALLBACK, FMI_SD_CARD, (INT32)isr_card_remove, (INT32)isr_card_insert);	
	#if 0	
		sprintf(szAsciiName, "C:\\Test.txt");
		hFile = AudioOpenFile(szAsciiName);    
		AudioWriteFileData(hFile, 0, 0x10);									
		AudioWriteFileClose(hFile) ;   									
	#endif	
	}	
#ifdef __ENABLE_CACHE__	
	sysDisableCache(); 	
	sysFlushCache(I_D_CACHE);		
	sysEnableCache(CACHE_WRITE_BACK);
#else
	sysDisableCache(); 		
#endif 

	
    	sysSetLocalInterrupt(ENABLE_FIQ_IRQ);	
}    	

volatile BOOL bIsAudioSampleDone = FALSE;
INT32 i32Count = 0;
void AudioRecordSampleDone(void)
{
	i32Count = i32Count+1;
	bIsAudioSampleDone = TRUE;
}	

INT32 g_pi32AudSampleBuf[E_AUD_BUF];			/* Reserved 10s * 4 * 192000 */ 
INT32 main(void)
{
	unsigned char ucInput;
	E_AUR_MIC_SEL eMicType;
	UINT32 u32SamplingRate, u32RecTime;
	init();

	do{
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("						Please input MIC type        								  	\n");	
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("[0]. Mono LINE in. [1]. Mono MIC in. [2]. Mono Digital MIC in. [3]. Stereo Digital MIC im.\n");
		eMicType = (E_AUR_MIC_SEL)(sysGetChar()-0x30);
	}while(eMicType>eAUR_STEREO_DIGITAL_MIC_IN);
	
    	do{    	
#if 1	
		/* If DC bias from MIC_BIAS pin, it need to enable SPU also. Otherwise, the DC bias is only 1.8V */
		/* Do again after DrvAUR_Close() that will reset  ADC  Codec */
		DrvSPU_Open();
		spuDacOn(2);
		sysDelay(30);
		spuSetDacSlaveMode();
#endif
		DBG_PRINTF("================================================================\n");
		DBG_PRINTF("						Audio Record Demo Code									  	\n");	
		DBG_PRINTF("================================================================\n");					
		DBG_PRINTF("Please input sampling rate \n");
		DBG_PRINTF("[0]. 48000. [1]. 44100. [2]. 32000. [3]. 24000. [4]. 22050. [5]. 16000. \n");
		DBG_PRINTF("[6]. 12000. [7]. 11025. [8]. 8000. [9]. 96000. [A]. 192000. \n");
		ucInput = sysGetChar();
		switch(ucInput){
			case '0':	u32SamplingRate = 48000;		break;	
			case '1':	u32SamplingRate = 44100;		break;	
			case '2':	u32SamplingRate = 32000;		break;	
			case '3':	u32SamplingRate = 24000;		break;	
			case '4':	u32SamplingRate = 22050;		break;	
			case '5':	u32SamplingRate = 16000;		break;	
			case '6':	u32SamplingRate = 12000;		break;	
			case '7':	u32SamplingRate = 11025;		break;	
			case '8':	u32SamplingRate = 8000;		break;	
			case '9':	u32SamplingRate = 96000;		break;	
			case 'A':	u32SamplingRate = 192000;	break;	
			default :	u32SamplingRate = 48000;		break;	
		}		
		DBG_PRINTF("Please input recording time\n");
		DBG_PRINTF("[0]. 5s. [1]. 10s. [2]. 30s. [3]. 1min.\n");
		ucInput = sysGetChar();
		switch(ucInput){
			case '0':	u32RecTime = 5;		break;	
			case '1':	u32RecTime = 10;		break;	
			case '2':	u32RecTime = 30;		break;	
			case '3':	u32RecTime = 60;		break;				
		}
		Emu_AudioRecordAnalogMIC_EDMA_Sample16Bit_AGC( eMicType, u32SamplingRate,  u32RecTime);	//OK	
										
	}while((ucInput!= 'q') || (ucInput!= 'Q'));
}
