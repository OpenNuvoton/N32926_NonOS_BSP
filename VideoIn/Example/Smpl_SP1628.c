#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "DrvI2C.h"
#include "demo.h"

struct SP_RegValue
{
	UINT8	u8RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};
#define _REG_TABLE_SIZE(nTableName)	sizeof(nTableName)/sizeof(struct SP_RegValue)

#define REG_VALUE_INIT	0
#define REG_VALUE_VGA		1	//640X480
#define REG_VALUE_SVGA	2	//800X600
#define REG_VALUE_HD720	3	//1280X720



#define DrvVideoIn_nt99140	1
#define CHIP_VERSION_H		0x02
#define CHIP_VERSION_L		0xA0
#define NT99140_CHIP_ID	0x

struct SP_RegTable{
	struct SP_RegValue *sRegTable;
	UINT16 uTableSize;
};

extern UINT8 u8PlanarFrameBuffer[];

struct SP_RegValue g_sSP1628_Init[] = 
{
	{0, 0}
};


struct SP_RegValue g_sSP1628_HD720[] = 
{
	#include "SP1628\SP1628_72M_720p_fix 25fps.dat"			//6/17	
	//#include "SP1628\SP1628_72M_720p_25_28fps.dat" 	//6/16
	//#include "SP1628\SP1628_60M_720p_18_20fps_8.dat"
	//#include "SP1628\SP1628_48M_720p_16_18fps_9.dat"
	//#include "SP1628\_SP1628_48M_720p_16_18fps_9.dat"
};
struct SP_RegValue g_sSP1628_SVGA[] = 
{
	//{0, 0}
	#include "SP1628\SP1628_48M_720p_16_18fps_9.dat"
};
struct SP_RegValue g_sSP1628_VGA[] = 
{
	//{0, 0}
	#include "SP1628\SP1628_48M_720p_16_18fps_9.dat"
};

struct SP_RegTable g_SP1628_InitTable[] =
{

	{g_sSP1628_Init,_REG_TABLE_SIZE(g_sSP1628_Init)},
	{g_sSP1628_VGA, _REG_TABLE_SIZE(g_sSP1628_VGA)},	
	{g_sSP1628_SVGA,_REG_TABLE_SIZE(g_sSP1628_SVGA)},
	{g_sSP1628_HD720,_REG_TABLE_SIZE(g_sSP1628_HD720)},		

	{0,0}
};


extern UINT8 u8DiffBuf[];
extern UINT8 u8OutLumBuf[];

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
	0x00			// not a device ID
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
#if 1//Normal	
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 set high default
	gpio_setportpull(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 pull-up 
	gpio_setportdir(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 output mode 
	sysDelay(3);			
	gpio_setportval(GPIO_PORTB, 1<<4, 0<<4);	//GPIOB 4 set low
	sysDelay(3);				
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOb 4 set high
#else
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 set high default
	gpio_setportpull(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 pull-up 
	gpio_setportdir(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 output mode 
	sysDelay(3);			
	gpio_setportval(GPIO_PORTB, 1<<4, 0<<4);	//GPIOB 4 set low
	sysDelay(3);				
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOb 4 set high	
	
	
	//Test
	sysDelay(3);	
	gpio_setportval(GPIO_PORTB, 1<<4, 0<<4);	//GPIOB 4 set low		
#endif	
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPE8 power down, HIGH for power down */
	//gpio_open(GPIO_PORTB);						//GPIOB as GPIO
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);		//GPIOB 3 set high default
	gpio_setportpull(GPIO_PORTE, 1<<8, 1<<8);		//GPIOB 3 pull-up 
	gpio_setportdir(GPIO_PORTE, 1<<8, 1<<8);		//GPIOB 3 output mode 				
	sysDelay(1);
	if(bIsEnable){
		gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOB 3 set high		
		sysDelay(5);
		gpio_setportval(GPIO_PORTE, 1<<8, 0);		//GPIOB 3 set low			
		sysDelay(5);
		gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOB 3 set high	
	}else{				
		gpio_setportval(GPIO_PORTE, 0<<8, 0);		//GPIOB 3 set low
		sysDelay(5);
		gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOB 3 set high
		sysDelay(5);
		gpio_setportval(GPIO_PORTE, 1<<8, 0);		//GPIOB 3 set low		
	}	
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

VOID SP1628_Init(UINT32 nIndex, UINT32 u32Resolution)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 id0, id1;
	struct SP_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;	
	sysDelay(2);
	
#ifdef __DEV__		
	SnrPowerDown(FALSE); 	 	/* DEV use power down pin, demo board force it to low state for normal run */	
#endif 
	SnrReset();	
	
	sysDelay(2);
	u32TableSize = g_SP1628_InitTable[0].uTableSize;
	psRegValue = g_SP1628_InitTable[0].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 ){
		sysprintf("Non sensor initial table\n");
		return;	
	}
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
	if(u32TableSize!=1){							
		for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
		{	
			sysprintf("Initial table\n");
			I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));						
		}
	}
	id0 = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID,CHIP_VERSION_H);
	id1 = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID,CHIP_VERSION_L);
	//static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
	sysprintf("Detectd sensor id0=%0x id1=%02x\n",id0, id1);
	
	u32TableSize = g_SP1628_InitTable[u32Resolution].uTableSize;
	psRegValue = g_SP1628_InitTable[u32Resolution].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));						
	}

	DrvI2C_Close();	
}

extern UINT8 u8PlanarFrameBuffer[];

/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/

UINT32 Smpl_SP1628_HD(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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
	pVin->Open(48000, 24000);//30000);	/* Sensor clock 24MHz--> PCLK = 72MHz */
	SP1628_Init(12, REG_VALUE_HD720);			
		
	pVin->EnableInt(eVIDEOIN_VINT);
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt							
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);		
	
#if 0												
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_UYVY, 				//NT99050
								eVIDEOIN_IN_YUV422	,	//Intput format
								eVIDEOIN_OUT_YUV422);		//Output format for packet
#else
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_VYUY, 				//
								eVIDEOIN_IN_YUV422	,	//Intput format
								eVIDEOIN_OUT_YUV422);		//Output format for packet
#endif								
	pVin->SetCropWinStartAddr(0,							//Horizontal start position	
							0);						//Useless
	
	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
#if 1
 	pVin->SetSensorPolarity(FALSE,							
						FALSE,						
						TRUE);							
#else
	pVin->SetSensorPolarity(TRUE,							
						FALSE,						
						FALSE);	
#endif						
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
