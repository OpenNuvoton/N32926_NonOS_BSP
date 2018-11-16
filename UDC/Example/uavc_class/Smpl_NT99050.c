#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "demo.h"
#include "DrvI2C.h"

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

struct NT_RegValue
{
	UINT16	u16RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};

struct NT_RegTable{
	struct NT_RegValue *sRegTable;
	UINT32 u32TableSize;
};

#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct NT_RegValue))

struct NT_RegValue g_sNT99050_RegValue[] = 
{
 	//[InitialSetting]
    {0x3021, 0x01},
    {0x32F0, 0x01}, 
    {0x3024, 0x00},
    {0x3270, 0x00}, //[Gamma_MDR]
    {0x3271, 0x0D}, 
    {0x3272, 0x19}, 
    {0x3273, 0x2A}, 
    {0x3274, 0x3C}, 
    {0x3275, 0x4D}, 
    {0x3276, 0x67}, 
    {0x3277, 0x81}, 
    {0x3278, 0x98}, 
    {0x3279, 0xAD}, 
    {0x327A, 0xCE}, 
    {0x327B, 0xE0}, 
    {0x327C, 0xED}, 
    {0x327D, 0xFF}, 
    {0x327E, 0xFF}, 
    {0x3060, 0x01},
    {0x3210, 0x04}, //LSC //D
    {0x3211, 0x04}, //F
    {0x3212, 0x04}, //D
    {0x3213, 0x04}, //D
    {0x3214, 0x04},
    {0x3215, 0x05},
    {0x3216, 0x04},
    {0x3217, 0x04},
    {0x321C, 0x04},
    {0x321D, 0x05},
    {0x321E, 0x04},
    {0x321F, 0x03},
    {0x3220, 0x00},
    {0x3221, 0xA0},
    {0x3222, 0x00},
    {0x3223, 0xA0},
    {0x3224, 0x00},
    {0x3225, 0xA0},
    {0x3226, 0x80},
    {0x3227, 0x88},
    {0x3228, 0x88},
    {0x3229, 0x30},
    {0x322A, 0xCF},
    {0x322B, 0x07},
    {0x322C, 0x04},
    {0x322D, 0x02},
    {0x3302, 0x00},//[CC: Saturation:100%]
    {0x3303, 0x1C},
    {0x3304, 0x00},
    {0x3305, 0xC8},
    {0x3306, 0x00},
    {0x3307, 0x1C},
    {0x3308, 0x07},
    {0x3309, 0xE9},
    {0x330A, 0x06},
    {0x330B, 0xDF},
    {0x330C, 0x01},
    {0x330D, 0x38},
    {0x330E, 0x00},
    {0x330F, 0xC6},
    {0x3310, 0x07},
    {0x3311, 0x3F},
    {0x3312, 0x07},
    {0x3313, 0xFC},
    {0x3257, 0x50}, //CA Setting
    {0x3258, 0x10},
    {0x3251, 0x01},  
    {0x3252, 0x50},  
    {0x3253, 0x9A},  
    {0x3254, 0x00}, 
    {0x3255, 0xd8},  
    {0x3256, 0x60}, 
    {0x32C4, 0x38}, 
    {0x32F6, 0xCF},
    {0x3363, 0x37},
    {0x3331, 0x08},
    {0x3332, 0x6C}, // 60
    {0x3360, 0x10},
    {0x3361, 0x30},
    {0x3362, 0x70},
    {0x3367, 0x40},
    {0x3368, 0x32}, //20
    {0x3369, 0x24}, //1D
    {0x336A, 0x1A},
    {0x336B, 0x20},
    {0x336E, 0x1A},
    {0x336F, 0x16},
    {0x3370, 0x0c},
    {0x3371, 0x12},
    {0x3372, 0x1d},
    {0x3373, 0x24},
    {0x3374, 0x30},
    {0x3375, 0x0A},
    {0x3376, 0x18},
    {0x3377, 0x20},
    {0x3378, 0x30},
    {0x3340, 0x1C},
    {0x3326, 0x03}, //Eext_DIV
    {0x3200, 0x3E}, //1E
    {0x3201, 0x3F},
    {0x3109, 0x82}, //LDO Open
    {0x3106, 0x07},
    {0x303F, 0x02},
    {0x3040, 0xFF},
    {0x3041, 0x01},
    {0x3051, 0xE0},
    {0x3060, 0x01},


      {0x32BF,0x04},
		{0x32C0,0x6A},
		{0x32C1,0x6A},
		{0x32C2,0x6A},
		{0x32C3,0x00},
		{0x32C4,0x20},
		{0x32C5,0x20},
		{0x32C6,0x20},
		{0x32C7,0x00},
		{0x32C8,0x95},
		{0x32C9,0x6A},
		{0x32CA,0x8A},
		{0x32CB,0x8A},
		{0x32CC,0x8A},
		{0x32CD,0x8A},
		{0x32D0,0x01},
		{0x3200,0x3E},
		{0x3201,0x0F},
		{0x302A,0x00},
		{0x302B,0x09},
		{0x302C,0x00},
		{0x302D,0x04},
		{0x3022,0x24},
		{0x3023,0x24},
		{0x3002,0x00},
		{0x3003,0x00},
		{0x3004,0x00},
		{0x3005,0x00},
		{0x3006,0x02},
		{0x3007,0x83},
		{0x3008,0x01},
		{0x3009,0xE3},
		{0x300A,0x03},
		{0x300B,0x28},
		{0x300C,0x01},
		{0x300D,0xF4},
		{0x300E,0x02},
		{0x300F,0x84},
		{0x3010,0x01},
		{0x3011,0xE4},
		{0x32B8,0x3B},
		{0x32B9,0x2D},
		{0x32BB,0x87},
		{0x32BC,0x34},
		{0x32BD,0x38},
		{0x32BE,0x30},
		{0x3201,0x3F},
		{0x320A,0x01},
		{0x3021,0x06},
		{0x3060,0x01},



};

static struct NT_RegTable g_NT99050_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{0, 0},
	{0, 0},//{g_sOV6880_RegValue,	_REG_TABLE_SIZE(g_sOV6880_RegValue)},		
	{0, 0},//{g_sOV7648_RegValue,	_REG_TABLE_SIZE(g_sOV7648_RegValue)},
	{0,0},
	{0, 0},//{g_sOV2640_RegValue,	_REG_TABLE_SIZE(g_sOV2640_RegValue)},	
	{0, 0},//{g_sOV9660_RegValue,	_REG_TABLE_SIZE(g_sOV9660_RegValue)},
	{g_sNT99050_RegValue,	_REG_TABLE_SIZE(g_sNT99050_RegValue)},
	{0, 0}
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
	0x00		// not a device ID
};


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
	gpio_setportval(GPIO_PORTB, 1<<4, 1<<4);	//GPIOB 4 set high
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPE8 power down, HIGH for power down */	
	outp32(REG_GPEFUN1, inp32(REG_GPEFUN1) & (~ MF_GPE8));
	gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);		//GPIOE 8 set high default
	gpio_setportpull(GPIO_PORTE, 1<<8, 1<<8);		//GPIOE 8 pull-up 
	gpio_setportdir(GPIO_PORTE, 1<<8, 1<<8);		//GPIOE 8 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTE, 1<<8, 1<<8);	//GPIOE 8 set high
	else				
		gpio_setportval(GPIO_PORTE, 1<<8, 0);		//GPIOE 8 set low		
}


BOOL I2C_Write_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr, UINT8 uData)
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

	if (uRegAddr==0x12 && (uData&0x80)!=0)
	{
		sysDelay(2);			
	}
	return TRUE;
}

UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
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


#define RETRY	1

#define CHIP_VERSION_H		0x3000  	/* Default 0x05 */
#define CHIP_VERSION_L		0x3001	/* Default 0x0 */
extern void Delay(UINT32 nCount);
VOID NT99050_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8IDH, u8IDL;
#if 0	
	INT32 rtval, j;
#endif	
	struct NT_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;	
	
	sysDelay(1);
	
	SnrReset();
	SnrPowerDown(FALSE); 	 	
				
	sysDelay(1);
	u32TableSize = g_NT99050_InitTable[nIndex].u32TableSize;
	psRegValue = g_NT99050_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	
#if 1
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB13));
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB14));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		//printf("Addr = 0x%x\n",  (psRegValue->u16RegAddr));
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));
			
		#if 0 
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			Delay(1000);		
			DBG_PRINTF("Delay A loop\n");
		}
		#else
		if ((psRegValue->u16RegAddr)==0x3021 && (psRegValue->u8Value)==0x01)
		{	
			//Delay(1000);
			sysDelay(2);		
			DBG_PRINTF("Delay A loop\n");
		}	
		#endif				
	}

	u8IDH = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_H);
	u8IDL = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_L);
	//static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
	sysprintf("Detectd sensor id0=%0x id1=%02x\n",u8IDH, u8IDL);	
	DrvI2C_Close();			
	
#else		
	/* Hardware I2C use GPIOB 13,14 */	
	i2cInit();	
	/* Byte Write/Random Read */
	rtval = i2cOpen();
	if(rtval < 0)
	{
		DBG_PRINTF("Open I2C error!\n");
		return;
	}	
	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, (u8DeviceID>>1), 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 200, 0);	
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u16RegAddr), 2); /* 2 means 16 bit */
		while(j-- > 0) 
		{
			//Delay(100);
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
			{
				break;
			}
			else
				sysprintf("WRITE ERROR 1 [%d]!\n", u32Idx);	
		}						
		if(j < 0)
			sysprintf("WRITE ERROR 2 [%d]!\n", u32Idx);			
	}	
	sysDelay(1);
	
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u16RegAddr), 2); /* 2 means 16 bit */
		while(j-- > 0) 
		{
			//Delay(100);
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
				break;
		}						
		if(j < 0)
			sysprintf("WRITE ERROR [%d]!\n", u32Idx);			
	}		
	
	/* Read IDH */
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, CHIP_VERSION_H, 2);	/* 2 bytes */
	j = RETRY;
	while(j-- > 0) {
		if(i2cRead(&u8IDH, 1) == 1)
			break;
	}
	
	/* Read IDL */
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, CHIP_VERSION_L, 2);	/* 2 bytes */	
	j = RETRY;
	while(j-- > 0) {
		if(i2cRead(&u8IDL, 1) == 1)
			break;
	}
	sysprintf("Detectd sensor IDH=%0x, IDL =%02x\n",u8IDH, u8IDL);
#endif	
}

UINT32 Smpl_NT99050(void)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;	
	INT32 i32ErrCode;
	outp32(REG_AHBCLK, 0xFFFFFFFF);

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
	NT99050_Init(6);			
	pVin->EnableInt(eVIDEOIN_VINT);
	
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt							
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);		
												
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_UYVY, 				//NT99050
								eVIDEOIN_IN_YUV422	,	//Intput format
								eVIDEOIN_OUT_YUV422);		//Output format for packet
	pVin->SetCropWinStartAddr(0,							//Horizontal start position	
							0);						//Useless
	
	pVin->SetStandardCCIR656(FALSE);						//standard CCIR656 mode		
 	pVin->SetSensorPolarity(FALSE,							
						FALSE,						
						TRUE);							
	pVin->SetCropWinSize(OPT_CROP_HEIGHT,				//UINT16 u16Height, 
						OPT_CROP_WIDTH);				//UINT16 u16Width
	
	pVin->PreviewPipeSize(OPT_PREVIEW_HEIGHT, OPT_PREVIEW_WIDTH);						
	pVin->EncodePipeSize(OPT_ENCODE_HEIGHT, OPT_ENCODE_WIDTH);
#ifdef __TV__
	pVin->SetStride(OPT_STRIDE, OPT_ENCODE_WIDTH);
	pVin->SetBaseStartAddress(eVIDEOIN_PACKET, 0, (UINT32)((UINT32)u8PacketFrameBuffer0) );
#else			
	pVin->SetStride(OPT_STRIDE, OPT_ENCODE_WIDTH);
	pVin->SetBaseStartAddress(eVIDEOIN_PACKET, (E_VIDEOIN_BUFFER)0, (UINT32)((UINT32)u8PacketFrameBuffer0 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );					
#endif			
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							(E_VIDEOIN_BUFFER)0, 							//Planar buffer Y addrress
							(UINT32)u8PlanarFrameBuffer0);
	
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							(E_VIDEOIN_BUFFER)1, 							//Planar buffer U addrress
							(UINT32)u8PlanarFrameBuffer0+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);
				
	pVin->SetPlanarFormat(eVIDEOIN_PLANAR_YUV422);				// Planar YUV422/420/macro 					
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							(E_VIDEOIN_BUFFER)2, 							//Planar buffer V addrress
							(UINT32)u8PlanarFrameBuffer0+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT/2);							
	pVin->SetPipeEnable(TRUE, 									// Engine enable?
						eVIDEOIN_BOTH_PIPE_ENABLE);		// which packet was enabled. 											
										
	pVin->SetShadowRegister();				
	sysSetLocalInterrupt(ENABLE_IRQ);						
														
	return Successful;			
}	
