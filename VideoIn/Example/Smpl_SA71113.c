#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "demo.h"
#include "DrvI2C.h"
//#include "w55fa92_i2c.h"

typedef struct
{
	UINT32 u32Width;
	UINT32 u32Height;
	char* pszFileName;
}S_VIDEOIN_REAL;
typedef struct
{
	UINT32 u32Width;
	UINT32 u32Height;
	E_VIDEOIN_OUT_FORMAT eFormat;
	char* pszFileName;
}S_VIDEOIN_PACKET_FMT;
#define OV7725_CCIR656
struct OV_RegValue
{
	UINT8	u8RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};

struct OV_RegTable{
    struct OV_RegValue *sRegTable;
	UINT32 u32TableSize;
};

#define REG_VALUE_INIT		0


#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct OV_RegValue))

static struct OV_RegValue g_sSA71113_Init[]=
{
	#include "SA71113\SA71113.dat"
};

static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{g_sSA71113_Init,_REG_TABLE_SIZE(g_sSA71113_Init)},
	{0, 0}
};

static UINT8 g_uOvDeviceID[]= 
{
	0x00,		// not a device ID
	0xc0,		// ov6680
	0x42,		// ov7648
	0x42,		// ov7670
	0x60,		// ov2640
	0x60,		// 0v9660
	0x42,		// NT99050 = 6
	0x54,		// NT99140 = 7
	0x54,		// NT99252 = 8
	0x54,		// NT99340 = 9
	0x6C,		// NT99252 = 10
	0x48,		// HM1375 = 11
	0x78,		// SP1628 = 12
	0x88	,		// TW9912 = 13	/* the slave address is 0x88 if pin SIAD to low, otherwise, the slave address is 0x8A if SIAD to high */
	0x4A			// SA71113 = 14
};


#ifdef __DEV__
/*
	Sensor power down and reset may default control on sensor daughter board and Reset by RC.	
	Sensor alway power on (Keep low)
*/
static void SnrReset(void)
{/* GPB04 reset:	H->L->H 	*/				
	//gpio_open(GPIO_PORTB);					//GPIOB4 as GPIO		
	outp32(REG_GPBFUN0, inp32(REG_GPBFUN0) & (~ MF_GPB4));
	
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 set high default
	gpio_setportpull(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 pull-up 
	gpio_setportdir(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 output mode 
	sysDelay(3);			
	gpio_setportval(GPIO_PORTB, 1<<4, 0<<4);	//GPIOB 4 set low
	sysDelay(3);				
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOb 4 set high
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPE8 power down, HIGH for power down */
	//gpio_open(GPIO_PORTB);						//GPIOE as GPIO
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);		//GPIOE 8 set high default
	gpio_setportpull(GPIO_PORTE, 1<<8, 1<<8);		//GPIOE 8 pull-up 
	gpio_setportdir(GPIO_PORTE, 1<<8, 1<<8);		//GPIOE 8 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 set high
	else				
		gpio_setportval(GPIO_PORTE, 1<<8, 0);		//GPIOE 8 set low		
}
#endif 
#ifdef __DEMO__
static void SnrReset(void)
{/*  GPE8 reset  */				
	//gpio_open(GPIO_PORTB);						//GPIOE as GPIO	
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 set high default
	gpio_setportpull(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 pull-up 
	gpio_setportdir(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 output mode 
	sysDelay(3);			
	gpio_setportval(GPIO_PORTE, 1<<8, 0<<8);	//GPIOE 8 set low
	sysDelay(3);				
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 set high
}
#endif


#define RETRY	2
VOID SA71113_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	struct OV_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;					

#ifdef __DEV__	 
	SnrPowerDown(FALSE);	 
#endif 	 				
	SnrReset();										
		
	u32TableSize = g_OV_InitTable[REG_VALUE_INIT].u32TableSize;
	psRegValue = g_OV_InitTable[REG_VALUE_INIT].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	
		
#ifdef __DEV__
	#if defined(__1ST_PORT__) || defined(__2ND_PORT__)
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB13));
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB14));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
	#endif			
#endif		
#ifdef __DEMO__
	#if defined(__1ST_PORT__) 
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB13));
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB14));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);	
	#endif
	#if defined(__2ND_PORT__) 
	outp32(REG_GPBFUN0, inp32(REG_GPBFUN0) & (~MF_GPB4));
	outp32(REG_GPAFUN1, inp32(REG_GPAFUN1) & (~MF_GPA12) | (2<<16));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN4, 
				eDRVGPIO_GPIOA,
				eDRVGPIO_PIN12, 
				(PFN_DRVI2C_TIMEDELY)Delay);	
	outp32(REG_GPAFUN1, inp32(REG_GPAFUN1) & (~MF_GPA12) | (2<<16));				
	#endif			
#endif	
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++){
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));	
		Delay(1000);					
	}		

	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x0); //Device ID will read back 0x60
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	
	DrvI2C_Close();	
}
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
void videoIn0_SetFieldDetection(
	BOOL 	bDetPosition,					 	 	
	BOOL 	bFieldDetMethod						//0: Detect field by Vsync & Hsync(SA711113). 1:Detect field by field pin(WT8861)	 
);

extern UINT32 u32PlanarIdx;
UINT32 Smpl_SA71113_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;
	INT32 i;
	INT32 i32ErrCode;
	UINT32 mode;
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	
	InitVPOST(pu8FrameBuffer0);
	
#ifdef __1ST_PORT__	
	i32ErrCode = register_vin_device(1, &Vin);	
	if(i32ErrCode<0){
		sysprintf("Register vin 0 device fail\n");
		return (UINT32)-1;
	}
	pVin = &Vin;
	pVin->Init(TRUE, (E_VIDEOIN_SNR_SRC)eSYS_UPLL, 24000, eVIDEOIN_SNR_CCIR601);
#endif 
#ifdef __2ND_PORT__	
	i32ErrCode = register_vin_device(2, &Vin);
	if(i32ErrCode<0){
		sysprintf("Register vin 1 device fail\n");
		return (UINT32)-1;
	}
	pVin = &Vin;
	pVin->Init(TRUE, (E_VIDEOIN_SNR_SRC)eSYS_UPLL, 24000, eVIDEOIN_2ND_SNR_CCIR601);
#endif	
	pVin->Open(96000, 48000);	
	SA71113_Init(14);			
	pVin->EnableInt(eVIDEOIN_VINT);
	
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt						
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);		
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_VYUY, //eVIDEOIN_IN_UYVY, //eVIDEOIN_IN_VYUY, 				//Input Order 
							eVIDEOIN_IN_YUV422,			//Intput format
							eVIDEOIN_OUT_YUV422);			//Output format for packet 																											
									
												
#if 1		
	pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
				eVIDEOIN_TYPE_CCIR601,	//0: CCIR601.	1: CCIR656 	
				1);						//swap?
	pVin->SetSensorPolarity(TRUE,				//V			
						FALSE,			//H			
						TRUE);			//P
	videoIn0_SetFieldDetection(0, 0);	
	pVin->SetCropWinStartAddr(3,								//Vertical start position 	Y
						0x38);							
	pVin->SetStandardCCIR656(TRUE);						
#else				
	pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
				eVIDEOIN_TYPE_CCIR656,	//0: CCIR601.	1: CCIR656 	
				1);						//swap?				
	videoIn0_SetFieldDetection(TRUE, 0);						
	pVin->SetSensorPolarity(FALSE,							
						FALSE,						
						TRUE);	
		pVin->SetCropWinStartAddr(40,								//Vertical start position 	Y
							0x38);						
 	//pVin->SetStandardCCIR656(FALSE);						//standard CCIR656 mode		
#endif 	
 									
				
	pVin->SetCropWinSize(OPT_CROP_HEIGHT,					//UINT16 u16Height, 
						OPT_CROP_WIDTH);					//UINT16 u16Width;					
	pVin->PreviewPipeSize(OPT_PREVIEW_HEIGHT, OPT_PREVIEW_WIDTH);						
	pVin->EncodePipeSize(OPT_ENCODE_HEIGHT, OPT_ENCODE_WIDTH);
#ifdef __TV__
	pVin->SetStride(OPT_STRIDE, OPT_ENCODE_WIDTH);
	pVin->SetBaseStartAddress(eVIDEOIN_PACKET, 0, (UINT32)((UINT32)pu8FrameBuffer0) );
#else			
	pVin->SetStride(OPT_STRIDE, OPT_ENCODE_WIDTH);
	pVin->SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)pu8FrameBuffer0 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );					
#endif			
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							(E_VIDEOIN_BUFFER)0, 							//Planar buffer Y addrress
							(UINT32)u8PlanarFrameBuffer);
	
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							(E_VIDEOIN_BUFFER)1, 							//Planar buffer U addrress
							(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);
				
	pVin->SetPlanarFormat(eVIDEOIN_PLANAR_YUV422);				// Planar YUV422/420/macro 					
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							(E_VIDEOIN_BUFFER)2, 							//Planar buffer V addrress
							(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/2);							
	pVin->SetPipeEnable(TRUE, 									// Engine enable?
						eVIDEOIN_BOTH_PIPE_ENABLE);		// which packet was enabled. 											
										
	pVin->SetShadowRegister();				
	sysSetLocalInterrupt(ENABLE_IRQ);		
	//
	mode = eVIDEOIN_TYPE_CCIR601;
	for(i=0; i<8;i=i+1)
	{	
		if(i==0){
			videoIn0_SetFieldDetection(0, 0);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					0);						//swap?
		}else if(i==1){
			videoIn0_SetFieldDetection(0, 0);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					1);						//swap?		
		}else if(i==2){
			#if 0
			videoIn0_SetFieldDetection(0, 1);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					0);	
			#endif		
		}else if(i==3){
			#if 0
			videoIn0_SetFieldDetection(0, 1);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					1);	
			#endif		
		}else if(i==4){
			videoIn0_SetFieldDetection(1, 0);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					0);	
		}else if(i==5){
			videoIn0_SetFieldDetection(1, 0);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					1);	
		}else if(i==6){
			#if 0
			videoIn0_SetFieldDetection(1, 1);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					0);	
			#endif		
		}else if(i==7){
			#if 0
			videoIn0_SetFieldDetection(1, 1);	
			pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
					mode,	//0: CCIR601.	1: CCIR656 	
					1);
			#endif		
		}
		sysDelay(60);
		pVin->SetOperationMode(TRUE);	
		while(pVin->GetOperationMode()==TRUE);
		pVin->SetPipeEnable(TRUE, eVIDEOIN_BOTH_PIPE_ENABLE);
	
		sysDelay(60);
		pVin->SetOperationMode(TRUE);	
		while(pVin->GetOperationMode()==TRUE);

		jpegOpen ();
		if(u32PlanarIdx==1)
			JpegEncTest(pVin, 0,  (UINT32)(&u8PlanarFrameBuffer));		/* Current 1, encode planar buffer 0*/
		else
			JpegEncTest(pVin, 0,  (UINT32)(&u8PlanarFrameBuffer1));	/* Current 0, encode planar buffer 1*/
		
		pVin->SetPipeEnable(TRUE, eVIDEOIN_BOTH_PIPE_ENABLE);
		sysDelay(30);				
	}								
	return Successful;			
}	


	