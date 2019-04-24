#include "wblib.h"
#include "wbtypes.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "demo.h"
#include "DrvI2C.h"
#include "GM7150\sensor_gm7150.h"



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
#define REG_VALUE_NTSC	1
#define REG_VALUE_PAL		2


#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct OV_RegValue))

static struct OV_RegValue g_sGM7150_Init[]=
{
	#include "GM7150\GM7150.dat"
};
static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{g_sGM7150_Init,_REG_TABLE_SIZE(g_sGM7150_Init)},
	{0, 0}
};

static UINT8 g_uOvDeviceID[]= 
{
	0xBA,			/* If pin SIAD high, 8 bits slave address = 0xBA */				
					/* If pin SIAD low, 8 bits slave address = 0xB8 */	
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




void GM7150SetVideoformat(int std)
{
	UINT8  uDeviceID;
	UINT8  fmt = 0;
	UINT8  ret;	
	
	uDeviceID  = g_uOvDeviceID[0];
	
	/* First tests should be against specific std */
	if (std == TVP_STD_ALL) {
		fmt=0;	/* Autodetect mode */
	} else if (std & TVP_STD_NTSC_443) {
		fmt=0xa;
	} else if (std & TVP_STD_PAL_M) {
		fmt=0x6;
	} else if (std & (TVP_STD_PAL_N| TVP_STD_PAL_Nc)) {
		fmt=0x8;
	} else {
		/* Then, test against generic ones */
		if (std & TVP_STD_NTSC) {
			fmt=0x2;
		} else if (std & TVP_STD_PAL) {
			fmt=0x4;
		}
	}
	sysprintf("GM7150 Video format : %02x. \r\n",fmt);
	ret = I2C_Write_8bitSlaveAddr_8bitReg_8bitData(uDeviceID, GM7150_VIDEO_STD, fmt);
	if(ret==FALSE)
		sysprintf("Programminmg GM7150 REG 0x%x fail\n", GM7150_VIDEO_STD);
	else
		sysprintf("Programminmg GM7150 REG 0x%x successful\n", GM7150_VIDEO_STD);
}

void GM7150SetInputSource(int src)
{
	UINT8  uDeviceID;
	UINT8  inputsrc = 0;
	
	uDeviceID  = g_uOvDeviceID[0];
	
	/* First tests should be against specific std */
	if (src == GM7150_A1P1A) {
		inputsrc=0x00;	/* A1P1A mode */
		sysprintf("GM7150 Video Input Source : %02x. GM7150 Channel = A1P1A mode. \r\n",inputsrc);
	} else if (src == GM7150_A1P1B) {
		inputsrc=0x02;	/* A1P1B mode */
		sysprintf("GM7150 Video Input Source : %02x. GM7150 Channel = A1P1B mode. \r\n",inputsrc);
	} else {
		inputsrc=0x01;	/* S-Video mode */
		sysprintf("GM7150 Video Input Source : %02x. GM7150 Channel = SVIDEO mode. \r\n",inputsrc);
	}

	I2C_Write_8bitSlaveAddr_8bitReg_8bitData(uDeviceID, GM7150_VD_IN_SRC_SEL_1, inputsrc);
}


#define RETRY	2
VOID GM7150_Init(UINT32 nIndex)
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
#ifdef __NTSC__		
	u32TableSize = g_OV_InitTable[REG_VALUE_NTSC].u32TableSize;
	psRegValue = g_OV_InitTable[REG_VALUE_NTSC].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	if ( psRegValue == 0 )
		return;	
#endif 
#ifdef __PAL__		
	u32TableSize = g_OV_InitTable[REG_VALUE_PAL].u32TableSize;
	psRegValue = g_OV_InitTable[REG_VALUE_PAL].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	if ( psRegValue == 0 )
		return;	
#endif 
		
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++){
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));	
		//Delay(1000);		
		//DBG_PRINTF("Delay A loop\n");			
	}
	
	
#if defined(__PAL__)
	GM7150SetVideoformat(TVP_STD_PAL);	
#elif defined(__NTSC__)
	GM7150SetVideoformat(TVP_STD_NTSC);
#else		// default PAL
	GM7150SetVideoformat(TVP_STD_PAL);	
#endif
	GM7150SetInputSource(GM7150_A1P1A);

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
UINT32 Smpl_GM7150_OneField(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;

	INT32 i32ErrCode;
	
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0xFF, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0xFF, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0xFF, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	
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
	pVin->Open(48000, 24000);	
	GM7150_Init(0);			
	pVin->EnableInt(eVIDEOIN_VINT);
	
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt						
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);	
	pVin->SetSensorPolarity(FALSE,
						FALSE,				//Polarity.	
						FALSE);		
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_VYUY, //eVIDEOIN_IN_VYUY, 				//Input Order 
							eVIDEOIN_IN_YUV422,			//Intput format
							eVIDEOIN_OUT_YUV422);			//Output format for packet 																											
	//pVin->SetCropWinStartAddr(0x20,								//Vertical start position 	Y
	//						0x7A);										
#ifdef __NTSC__	
	pVin->SetCropWinStartAddr(0x4,								//Vertical start position 	Y
							0x3);
#endif 
#ifdef __PAL__	
	pVin->SetCropWinStartAddr(0x4,								//Vertical start position 	Y
							0x3);	
#endif 							
												

	pVin->SetInputType(1,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
				eVIDEOIN_TYPE_CCIR656,	//0: CCIR601.	1: CCIR656 	
				0);						//swap?
 	pVin->SetStandardCCIR656(FALSE);						//standard CCIR656 mode		
									
				
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

	/* Buffer still keep VGA */ 
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


UINT32 Smpl_GM7150_TwoFields(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	PUINT8 pu8PacketBuf;

	INT32 i32ErrCode;
	
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer0 | 0x80000000);
	memset(pu8PacketBuf, 0xFF, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer1 | 0x80000000);
	memset(pu8PacketBuf, 0xFF, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	pu8PacketBuf = (PUINT8)((UINT32)pu8FrameBuffer2 | 0x80000000);
	memset(pu8PacketBuf, 0xFF, OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT*2);
	
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
	pVin->Open(48000, 24000);	
	GM7150_Init(0);			
	pVin->EnableInt(eVIDEOIN_VINT);
	
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt						
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);	
	pVin->SetSensorPolarity(FALSE,
						FALSE,				//Polarity.	
						FALSE);		
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_VYUY, //eVIDEOIN_IN_VYUY, 				//Input Order 
							eVIDEOIN_IN_YUV422,			//Intput format
							eVIDEOIN_OUT_YUV422);			//Output format for packet 																											
	//pVin->SetCropWinStartAddr(0x20,								//Vertical start position 	Y
	//						0x7A);										
#ifdef __NTSC__	
	pVin->SetCropWinStartAddr(0x23,								//Vertical start position 	Y
							0x6);	
#endif 
#ifdef __PAL__	
	pVin->SetCropWinStartAddr(0x23,								//Vertical start position 	Y
							0x6);	
#endif 							
												

	pVin->SetInputType(3,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
				eVIDEOIN_TYPE_CCIR656,	//0: CCIR601.	1: CCIR656 	
				0);						//swap?
 	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
									
				
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

	/* Buffer still keep VGA */ 
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


