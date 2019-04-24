#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "demo.h"
#include "w55fa92_i2c.h"

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

struct OV_RegValue
{
	UINT8	u8RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};

struct OV_RegTable{
    struct OV_RegValue *sRegTable;
	UINT32 u32TableSize;
};


#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct OV_RegValue))

static struct OV_RegValue g_sOV7725_VGA_RegValue[]=
{//OV7725		
#if 0
	{0x12 ,0x80}, {0x00 ,0x00}, {0x00 ,0x00}, {0x00 ,0x64}, {0x12 ,0x20}, {0x3D ,0x03}, {0x17 ,0x22}, {0x18 ,0xA4},
	{0x19 ,0x07}, {0x1A ,0xF0}, {0x32 ,0x02}, {0x29 ,0xA0}, {0x2C ,0xF0}, {0x2A ,0x02}, {0x65 ,0x20}, {0x11 ,0x01},
	{0x15 ,0x03}, {0x42 ,0x7F}, {0x63 ,0xE0}, {0x64 ,0xFF}, {0x66 ,0xC0}, {0x67 ,0x48}, {0x0D ,0x41}, {0x0E ,0x01},
	{0x0F ,0xC5}, {0x14 ,0x11}, {0x22 ,0x7f}, {0x23 ,0x03}, {0x24 ,0x40}, {0x25 ,0x30}, {0x26 ,0xa1}, {0x2B ,0x00},
	{0x6B ,0xAA}, {0x13 ,0xEF}, {0x90 ,0x05}, {0x91 ,0x01}, {0x92 ,0x03}, {0x93 ,0x00}, {0x94 ,0x90}, {0x95 ,0x8A},
	{0x96 ,0x06}, {0x97 ,0x0B}, {0x98 ,0x95}, {0x99 ,0xA0}, {0x9A ,0x1E}, {0x9B ,0x08}, {0x9C ,0x20}, {0x9E ,0x81},
	{0xA6 ,0x04}, {0x7E ,0x0C}, {0x7F ,0x24}, {0x80 ,0x3A}, {0x81 ,0x60}, {0x82 ,0x70}, {0x83 ,0x7E}, {0x84 ,0x8A},
	{0x85 ,0x94}, {0x86 ,0x9E}, {0x87 ,0xA8}, {0x88 ,0xB4}, {0x89 ,0xBE}, {0x8A ,0xCA}, {0x8B ,0xD8}, {0x8C ,0xE2},
	{0x8D ,0x28}, {0x46 ,0x05}, {0x47 ,0x00}, {0x48 ,0x00}, {0x49 ,0x12}, {0x4a ,0x00}, {0x4b ,0x13}, {0x4c ,0x21},
	{0x0C ,0x00}, {0x09 ,0x00}, {0xff ,0xff}, {0xff ,0xff}
#else
    {0x12, 0x80},      
    {0x3d, 0x03},     
    {0x17, 0x22},  
    {0x18, 0xa4},  
    {0x19, 0x07},  
    {0x1a, 0xf0},  
    {0x32, 0x00},  
    {0x29, 0xa0},  
    {0x2c, 0xf0},  
    {0x2a, 0x00},  
    {0x11, 0x01},//00/01/03/07 for 60/30/15/7.5fps   
      
    {0x42, 0x7f},  
    {0x4d, 0x09},  
    {0x63, 0xe0},  
    {0x64, 0xff},  
    {0x65, 0x20},  
    {0x66, 0x00},  
    {0x67, 0x48},      
      
    {0x13, 0xf0},      
    {0x0d, 0x41},//0x51/0x61/0x71 for different AEC/AGC window   
    {0x0f, 0xc5},  
    {0x14, 0x11},//0x81   
    {0x22, 0x7f},//ff/7f/3f/1f for 60/30/15/7.5fps   
    {0x23, 0x03},//01/03/07/0f for 60/30/15/7.5fps   
    {0x24, 0x50},//0x80   
    {0x25, 0x30},//5a   
    {0x26, 0xa1},//c1   
    {0x2b, 0x00},//ff   
    {0x6b, 0xaa},  
    {0x13, 0xff},  
      
    {0x90, 0x05},  
    {0x91, 0x01},  
    {0x92, 0x03},  
    {0x93, 0x00},  
  
    {0x94, 0xb0},  
    {0x95, 0x9d},  
    {0x96, 0x13},  
    {0x97, 0x16},  
    {0x98, 0x7b},  
    {0x99, 0x91},  
    {0x9a, 0x1e},  
      
    {0x9b, 0x00},  
    {0x9c, 0x25},  
    {0x9e, 0x81},  
    {0xa6, 0x06},  
  
//modified saturation initialization value by pw 2008-03-04   
    {0xa7, 0x65},  
    {0xa8, 0x65},  
      
    {0x7e, 0x0c},  
    {0x7f, 0x16},  
    {0x80, 0x2a},  
    {0x81, 0x4e},  
    {0x82, 0x61},  
    {0x83, 0x6f},  
    {0x84, 0x7b},  
    {0x85, 0x86},  
    {0x86, 0x8e},  
    {0x87, 0x97},  
    {0x88, 0xa4},  
    {0x89, 0xaf},  
    {0x8a, 0xc5},  
    {0x8b, 0xd7},  
    {0x8c, 0xe8},  
    {0x8d, 0x20},  
        
      
#if 0   
    {0x34, 0x00},  
    {0x33, 0x40},//0x66/0x99   
    {0x22, 0x99},  
    {0x23, 0x03},  
    {0x4a, 0x10},  
    {0x49, 0x10},  
    {0x4b, 0x14},  
    {0x4c, 0x17},  
    {0x46, 0x05},  
    {0x0e, 0x65},  
#endif       
      
    {0x69, 0x5d},  
    {0x0c, 0x00},  
  
    {0x33, 0x3f},//0x66/0x99   
    {0x0e, 0x65},  
  	/* Remark by SW */
  	//sharpness strength modified by pw 2008-03-05   
    	//regvalue = gpio_sccb_read{I2C_OV7725, 0xac},  
    	//{0xac, 0xdfRvalue},  
    	{0x8f, 0x04},  	
  
#endif	
};

static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{0, 0},
	{0, 0},//{g_sOV6880_RegValue,	_REG_TABLE_SIZE(g_sOV6880_RegValue)},		
	{0, 0},//{g_sOV7648_RegValue,	_REG_TABLE_SIZE(g_sOV7648_RegValue)},
	{0, 0},//{g_sOV7670_RegValue,	_REG_TABLE_SIZE(g_sOV7670_RegValue)},
	{0, 0},//{g_sOV2640_RegValue,	_REG_TABLE_SIZE(g_sOV2640_RegValue)},	
	{0, 0},//{g_sOV9660_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV9660_VGA_RegValue)},
	{0, 0},//{g_sOV9660_SXGA_RegValue,		_REG_TABLE_SIZE(g_sOV9660_SXGA_RegValue)},
	{g_sOV7725_VGA_RegValue,		_REG_TABLE_SIZE(g_sOV7725_VGA_RegValue)},
	{0, 0}
};

static UINT8 g_uOvDeviceID[]= 
{
	0x00,		// not a device ID
	0xc0,		// ov6680
	0x42,		// ov7648
	0x42,		// ov7670
	0x60,		// ov2640
	0x60,		// 0v9660 VGA
	0x60,		// 0v9660 SXGA
	0x42,		// 0v7725 VGA
	0x00			// not a device ID
};


#if 0
/*
	Sensor power down and reset may default control on sensor daughter board.
	Reset by RC.
	Sensor alway power on (Keep low)

*/
static void SnrReset(void)
{/* GPA11 reset:	H->L->H 	*/			
#ifdef __GPIO_PIN__
	gpio_open(GPIO_PORTA, 11);					//GPIOA 10 as GPIO
#else	
	gpio_open(GPIO_PORTA);						//GPIOA 10 as GPIO
#endif		
	gpio_setportval(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 set high default
	gpio_setportpull(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 pull-up 
	gpio_setportdir(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 output mode 
	Delay(1000);			
	gpio_setportval(GPIO_PORTA, 1<<11, 1<<11);	//GPIOA 11 set low
	Delay(1000);				
	gpio_setportval(GPIO_PORTA, 1<<11, 0);		//GPIOA 11 set high
}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPA10 power down, Low for power down */
#ifdef __GPIO_PIN__
	gpio_open(GPIO_PORTA, 10);					//GPIOA 10 as GPIO
#else	
	gpio_open(GPIO_PORTA);						//GPIOA 10 as GPIO
#endif	
	gpio_setportval(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 set high default
	gpio_setportpull(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 pull-up 
	gpio_setportdir(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTA, 1<<10, 0);	//GPIOA 10 set high
	else			
		gpio_setportval(GPIO_PORTA, 1<<10, 1<<10);	//GPIOA 10 set low	
}
#endif 


INT getFitPreviewDimension(UINT32 u32Lcmw,
						UINT32 u32Lcmh,
						UINT32 u32Patw,
						UINT32 u32Path,  
						UINT32* pu32Previewwidth, 
						UINT32* pu32Previewheight)				
{//Assume sensor aspect ratio is 4:3
	float lcmwidth, lcmheight;
	int prewidth, preheight;
	float aspect1=(float)u32Path/u32Patw;	
	float invaspect1=(float)u32Patw/u32Path;		
	float aspect2;
	

	lcmwidth = (float)u32Lcmw;
	lcmheight = (float)u32Lcmh;
	aspect2 = lcmheight/lcmwidth;
	if(aspect2>=aspect1)
	{
		
		prewidth = lcmwidth; 
	
		if( (((int)(lcmwidth*aspect1))%4!=0) && ((((int)lcmwidth*aspect1)/4+1)*4<=640) )
			preheight = (INT32)((lcmwidth*aspect1)/4+1)*4;
		else
			preheight = (INT32)((lcmwidth*aspect1)/4)*4;		
		
	}
	else
	{
		DBG_PRINTF("Domenonate is height, fixed height\n");
		preheight = lcmheight; 
		if( (((int)(lcmheight*invaspect1))%4!=0) && ((((int)lcmheight*invaspect1)/4+1)*4<=480) )
			prewidth = (INT32)((lcmheight*invaspect1)/4+1)*4;
		else
			prewidth = (INT32)((lcmheight*invaspect1)/4)*4;		
	}	
	

	*pu32Previewwidth = prewidth;
	*pu32Previewheight = preheight;

	sysprintf("target width, height= (%d, %d)\n", *pu32Previewwidth , *pu32Previewheight);	
	return Successful;
}



#if 1
#define RETRY	1
VOID OV7725_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	INT32 rtval;
	INT j;	
	
	struct OV_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;
	pVin->Open(48000, 24000);								/* For sensor clock output */	
#if 0	//Sensor module default no power down and no reset. 
	SnrPowerDown(FALSE);
	SnrReset();		 
#endif 	 										
		
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	sysprintf("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	
	/* Software I2C use GPIOB 13,14 */	
	i2cInit();	
	/* Byte Write/Random Read */
	rtval = i2cOpen();
	if(rtval < 0)
	{
		DBG_PRINTF("Open I2C error!\n");
		return;
	}	
	i2cIoctl(I2C_IOC_SET_DEV_ADDRESS, (u8DeviceID>>1), 0);  
	i2cIoctl(I2C_IOC_SET_SPEED, 100, 0);	
	i2cIoctl(I2C_IOC_SET_SINGLE_MASTER, 1, 0); 
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		j = RETRY;
		i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, (psRegValue->u8RegAddr), 1);
		while(j-- > 0) 
		{
			if(i2cWrite(&(psRegValue->u8Value), 1) == 1)
				break;
		}						
		if(j < 0)
			sysprintf("WRITE ERROR [%d]!\n", u32Idx);
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			sysDelay(2);		
			DBG_PRINTF("Delay A loop\n");
		}					
	}	
	
	
  	//sharpness strength modified by pw 2008-03-05   
    	//regvalue = gpio_sccb_read{I2C_OV7725, 0xac},  
    	//{0xac, 0xdfRvalue},  
    	//{0x8f, 0x04},  	
    	
  
		
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x0A, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j <= 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x0A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x0B, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j <= 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x0B);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x1C, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x1C);		
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	
	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x1D, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x1D);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0xD7, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0xD7);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	i2cIoctl(I2C_IOC_SET_SUB_ADDRESS, 0x6A, 1);	
	j = RETRY;
	while(j-- > 0) 
	{
		if(i2cRead_OV(&u8ID, 1) == 1)
			break;
	}
	if(j < 0)
		DBG_PRINTF("Read ERROR [%x]!\n", 0x6A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
}
#else
extern ERRCODE
DrvI2C_Open(
	UINT32 u32SCKPortIndex,
	UINT32 u32SCKPinMask,
	UINT32 u32SDAPortIndex,
	UINT32 u32SDAPinMask,
	PFN_DRVI2C_TIMEDELY pfnDrvI2C_Delay	
);

extern void Delay( 
	UINT32 nCount 
);

static BOOL I2C_Write_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr, UINT8 uData)
{
	// 3-Phase(ID address, regiseter address, data(8bits)) write transmission
	volatile u32Delay = 0x100;
	DrvI2C_SendStart();
	while(u32Delay--);		
	if ( (DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8)==FALSE) ||			// Write ID address to sensor
		 (DrvI2C_WriteByte(uRegAddr,DrvI2C_Ack_Have,8)==FALSE) ||	// Write register address to sensor
		 (DrvI2C_WriteByte(uData,DrvI2C_Ack_Have,8)==FALSE) )		// Write data to sensor
	{
		DrvI2C_SendStop();
		return FALSE;
	}
	DrvI2C_SendStop();

	if (uRegAddr==0x12 && (uData&0x80)!=0)
	{
		Delay(1000);			
	}
	return TRUE;
}

static UINT8 I2C_Read_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr)
{
	UINT8 u8Data;
	
	// 2-Phase(ID address, register address) write transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	DrvI2C_WriteByte(uRegAddr,DrvI2C_Ack_Have,8);	// Write register address to sensor
	DrvI2C_SendStop();

	// 2-Phase(ID-address, data(8bits)) read transmission
	DrvI2C_SendStart();
	DrvI2C_WriteByte(uAddr|0x01,DrvI2C_Ack_Have,8);		// Write ID address to sensor
	u8Data = DrvI2C_ReadByte(DrvI2C_Ack_Have,8);		// Read data from sensor
	DrvI2C_SendStop();
	
	return u8Data;
}

static VOID OV7725_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	struct OV_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;
	videoIn_Open(48000, 24000);								/* For sensor clock output */	

#if 0	//Sensor module default no power down and no reset. 
	SnrPowerDown(FALSE);
	SnrReset();		 
#endif 	 										
		
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	

	gpio_open(GPIO_PORTB);				//GPIOB as GPIO

	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));	
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			Delay(1000);		
			DBG_PRINTF("Delay A loop\n");
		}				
	}
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x0A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x0b);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x1C);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x1D);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0xD7);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x6A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	
	DrvI2C_Close();	
}
#endif


UINT32 Smpl_OV7725(void)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;

	INT32 i32ErrCode;
			
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
	OV7725_Init(7);			
	pVin->EnableInt(eVIDEOIN_VINT);
	
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt						
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);	
	pVin->SetSensorPolarity(FALSE,
						FALSE,				//Polarity.	
						TRUE);		
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_VYUY, 				//Input Order 
							eVIDEOIN_IN_YUV422,			//Intput format
							eVIDEOIN_OUT_YUV422);			//Output format for packet 																											
	pVin->SetCropWinStartAddr(0,								//Vertical start position 	Y
							0);										
												
#ifdef OV7725_CCIR656			
	pVin->SetInputType(0,					//0: Both fields are disabled. 1: Field 1 enable. 2: Field 2 enable. 3: Both fields are enable
				eVIDEOIN_TYPE_CCIR656,	//0: CCIR601.	1: CCIR656 	
				0);						//swap?
	pVin->SetStandardCCIR656(FALSE);					
	pVin->SetSensorPolarity(FALSE,							// Inverse the SOF EOF
						FALSE,							// Inverse the SOL EOL
						TRUE);	
			
 #else
 	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
 	pVin->SetSensorPolarity(TRUE,							
						FALSE,						
						TRUE);									
#endif				
	pVin->SetCropWinSize(OPT_CROP_HEIGHT,					//UINT16 u16Height, 
						OPT_CROP_WIDTH);					//UINT16 u16Width;					
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




