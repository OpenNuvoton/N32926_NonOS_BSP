#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "DrvI2C.h"
#include "demo.h"

struct NT_RegValue
{
	UINT16	u16RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};
#define _REG_TABLE_SIZE(nTableName)	sizeof(nTableName)/sizeof(struct NT_RegValue)

#define REG_VALUE_INIT	0
#define REG_VALUE_VGA	1	//640X480
#define REG_VALUE_HD720	2	//1280X720



#define DrvVideoIn_nt99140	1
#define CHIP_VERSION_H		0x3038 
#define CHIP_VERSION_L		0x3039
#define OV10633_CHIP_ID_H	0x14
#define OV10633_CHIP_ID_L	0x20

struct NT_RegTable{
	struct NT_RegValue *sRegTable;
	UINT16 uTableSize;
};

extern UINT8 u8PlanarFrameBuffer[];

struct NT_RegValue g_sOV10633_Init[] = 
{
	#include "OV10633\OV10633_Init.dat"
};
struct NT_RegValue g_sOV10633_HD720[] = 
{
	#include "OV10633\OV10633_HD720.dat"				    
};
struct NT_RegValue g_sOV10633_VGA[] = 
{
	#include "OV10633\OV10633_VGA.dat"
};

struct NT_RegTable g_OV10633_InitTable[] =
{

	{g_sOV10633_Init,_REG_TABLE_SIZE(g_sOV10633_Init)},
	{g_sOV10633_VGA, _REG_TABLE_SIZE(g_sOV10633_VGA)},	
	{g_sOV10633_HD720,_REG_TABLE_SIZE(g_sOV10633_HD720)},		

	{0,0}
};


extern UINT8 u8DiffBuf[];
extern UINT8 u8OutLumBuf[];

static UINT8 g_uOvDeviceID[]= 
{
	0x60,		// OV10633 = 9
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
	sysDelay(1);			
	gpio_setportval(GPIO_PORTB, 1<<4, 0<<4);	//GPIOB 4 set low
	sysDelay(1);				
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOb 4 set high
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPE8 power down, HIGH for power down */
	//gpio_open(GPIO_PORTB);						//GPIOB as GPIO
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);		//GPIOB 3 set high default
	gpio_setportpull(GPIO_PORTE, 1<<8, 1<<8);		//GPIOB 3 pull-up 
	gpio_setportdir(GPIO_PORTE, 1<<8, 1<<8);		//GPIOB 3 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOB 3 set high
	else				
		gpio_setportval(GPIO_PORTE, 1<<8, 0);		//GPIOB 3 set low		
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
	sysDelay(1);			
	gpio_setportval(GPIO_PORTE, 1<<8, 0<<8);	//GPIOE 8 set low
	sysDelay(1);				
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 set high
}
#endif

static BOOL I2C_Write_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr, UINT8 uData)
{
	// 3-Phase(ID address, regiseter address, data(8bits)) write transmission
	volatile int u32Delay = 0x100;
	DrvI2C_SendStart();
	while(u32Delay--);		
	if ( (DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8)==FALSE) ||			// Write ID address to sensor
		 (DrvI2C_WriteByte((UINT8)(uRegAddr>>8),DrvI2C_Ack_Have,8)==FALSE) ||	// Write register address to sensor
		 (DrvI2C_WriteByte((UINT8)(uRegAddr&0xff),DrvI2C_Ack_Have,8)==FALSE) ||	// Write register address to sensor
		 (DrvI2C_WriteByte(uData,DrvI2C_Ack_Have,8)==FALSE) )		// Write data to sensor
	{
		sysprintf("wnoack Addr = 0x%x \n", uRegAddr);
		DrvI2C_SendStop();
		return FALSE;
	}
	DrvI2C_SendStop();

	return TRUE;
}

static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
{
	UINT8 u8Data;
	
	// 2-Phase(ID address, register address) write transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	DrvI2C_WriteByte((UINT8)(uRegAddr>>8),DrvI2C_Ack_Have,8);	// Write register address to sensor
	DrvI2C_WriteByte((UINT8)(uRegAddr&0xff),DrvI2C_Ack_Have,8);	// Write register address to sensor	
	DrvI2C_SendStop();

	// 2-Phase(ID-address, data(8bits)) read transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr|0x01,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	u8Data = DrvI2C_ReadByte(DrvI2C_Ack_Have,8);		// Read data from sensor
	DrvI2C_SendStop();
	
	return u8Data;

}

VOID OV10633_Init(UINT32 nIndex, UINT32 u32Resolution)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 id0, id1;

	struct NT_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;	
	sysDelay(2);	//It need 33300 sensor clock. The minimum sensor clock is 16MHz. ==> it is about 2.09ms
	
#ifdef __DEV__		
	SnrPowerDown(FALSE); 	 	/* DEV use power down pin, demo board force it to low state for normal run */				
#endif 
	SnrReset();	
	
	sysDelay(2);
	u32TableSize = g_OV10633_InitTable[0].uTableSize;
	psRegValue = g_OV10633_InitTable[0].sRegTable;
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
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		

		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));	
		if(psRegValue->u16RegAddr==0x103)
			sysDelay(10);
		#if 0	
		u8Data = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,	(psRegValue->u16RegAddr));				
		if(u8Data!= (psRegValue->u8Value))
			sysprintf("Error write sensor register 0x%x\n", (psRegValue->u16RegAddr));
		#endif	
	}
	id0 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_H);
	id1 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_L);
	//static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
	sysprintf("Detectd sensor id0=%0x id1=%02x\n",id0, id1);
	
	
	
	u32TableSize = g_OV10633_InitTable[u32Resolution].uTableSize;
	psRegValue = g_OV10633_InitTable[u32Resolution].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{	
	#if 0	
		UINT8 u8Data;
	#endif	
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));	
		//u32Delay = 0x10000;
		//while(u32Delay--);
	#if 0
		u8Data = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,	(psRegValue->u16RegAddr));				
		
		if(u8Data!= (psRegValue->u8Value))
			sysprintf("Error write sensor register address = 0x%x\n",  (psRegValue->u16RegAddr));					
		else
		{
			sysDelay(1);
			I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));	
			u8Data = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,	(psRegValue->u16RegAddr));				
			if(u8Data!= (psRegValue->u8Value))
				sysprintf("Error write sensor register address = 0x%x again\n",  (psRegValue->u16RegAddr));		
		}	
	#endif	
	}


	id1 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,0x3021);
	//static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
	sysprintf("Sensor-0x3021 = 0x%x\n",id1);
	DrvI2C_Close();	
}

extern UINT8 u8PlanarFrameBuffer[];
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_OV10633_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;
	
	INT32 i32ErrCode;
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	outp32(REG_AHBCLK, 0xFFFFFFFF);

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
	pVin->Open(72000, 24000);
	OV10633_Init(0, REG_VALUE_VGA);			
	pVin->EnableInt(eVIDEOIN_VINT);
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt							
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);		
												
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_UYVY, 				//OV10633
								eVIDEOIN_IN_YUV422	,	//Intput format
								eVIDEOIN_OUT_YUV422);		//Output format for packet
	pVin->SetCropWinStartAddr(0,							//Horizontal start position	
							0);						//Useless
	
	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
 	pVin->SetSensorPolarity(TRUE,							
						FALSE,						
						FALSE);							
	pVin->SetCropWinSize(OPT_CROP_HEIGHT,				//UINT16 u16Height, 
						OPT_CROP_WIDTH);				//UINT16 u16Width
	
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
																				
	return Successful;			
}	

/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/

UINT32 Smpl_OV10633_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;
	
	INT32 i32ErrCode;
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0x0, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	outp32(REG_AHBCLK, 0xFFFFFFFF);

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

	pVin->Open(72000, 20000);
	OV10633_Init(0, REG_VALUE_HD720);			
		
	pVin->EnableInt(eVIDEOIN_VINT);
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt							
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);		
	
												
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_UYVY, 				//OV10633
								eVIDEOIN_IN_YUV422	,	//Intput format
								eVIDEOIN_OUT_YUV422);		//Output format for packet
	pVin->SetCropWinStartAddr(0,							//Horizontal start position	
							0);						//Useless
	
	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
 	pVin->SetSensorPolarity(TRUE,							
						FALSE,						
						FALSE);							
	pVin->SetCropWinSize(OPT_CROP_HEIGHT,				//UINT16 u16Height, 
						OPT_CROP_WIDTH);				//UINT16 u16Width
	
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
	
#ifdef _MACRO_BLOCK_														
	pVin->SetPlanarFormat(eVIDEOIN_MACRO_PLANAR_YUV420);		// Planar YUV422/420/macro 	
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							2, 							//Planar buffer V addrress
							(UINT32)u8PlanarFrameBuffer+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);

#endif	
	
	pVin->SetPipeEnable(TRUE, 									// Engine enable?
						eVIDEOIN_BOTH_PIPE_ENABLE);		// which packet was enabled. 											
	
								
	pVin->SetShadowRegister();			
		
	sysSetLocalInterrupt(ENABLE_IRQ);							
														
	return Successful;			
}	


static INT32 IQ_GetBrightness(void)
{
	return 0;
}
static INT32 IQ_SetBrightness(INT16 i16Brightness)
{
	return 0;
}
static INT32 IQ_GetSharpness(void)
{
	return 0;
}
static INT32 IQ_SetSharpness(INT16 i16Brightness)
{
	return 0;
}
static INT32 IQ_GetContrast(void)
{
	return 0;
}
static INT32 IQ_SetContrast(INT16 i16Contrast)
{
	return 0;
}
static INT32 IQ_GetHue(void)
{
	UINT8 u8RegData; 
	
	UINT8 u8DeviceID = g_uOvDeviceID[0];
	
	u8RegData = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x32FB); 	
	return u8RegData;
}
static INT32 IQ_SetHue(INT16 i16Hue)
{
	
	UINT8 u8DeviceID = g_uOvDeviceID[0];
	
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x32F1, 0x05); 
	I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, 0x32FB, i16Hue); 	
	return Successful;
} 

IQ_S ImageQualityTbl =
{	
	IQ_GetBrightness, 		
	IQ_SetBrightness,
	IQ_GetSharpness,		
	IQ_SetSharpness,
	IQ_GetContrast,		
	IQ_SetContrast,
	IQ_GetHue,			
	IQ_SetHue,
};
INT32 register_sensor(IQ_S* ps_sensor)
{
	*ps_sensor = ImageQualityTbl;
	return Successful;	
}