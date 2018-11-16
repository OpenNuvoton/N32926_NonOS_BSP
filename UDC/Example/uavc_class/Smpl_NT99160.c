#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "demo.h"
#include "w55fa92_i2c.h"

#include "DrvI2C.h"

struct NT_RegValue
{
	UINT16	u16RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};
#define _REG_TABLE_SIZE(nTableName)	sizeof(nTableName)/sizeof(struct NT_RegValue)

#define REG_VALUE_INIT	0
#define REG_VALUE_VGA		1	//640X480
#define REG_VALUE_SVGA	2	//800X600
#define REG_VALUE_HD720	3	//1280X720



#define DrvVideoIn_nt99160	1
#define CHIP_VERSION_H		0x3000
#define CHIP_VERSION_L		0x3001
#define NT99160_CHIP_ID	0x14

struct NT_RegTable{
	struct NT_RegValue *sRegTable;
	UINT16 uTableSize;
};


extern VINDEV_T Vin;
extern VINDEV_T* pVin;

struct NT_RegValue g_sNT99160_Init[] = 
{
//Pwdn Pin = 0	; 0: normal mode 1: power down mode
//I2C Slave ID = 0x54 ; sensor slave ID address
//Hsyn  = Active-H
//Vsyn  = Active-H
//PCLK = Active-H
//Out Format = YUYV
//MCLK Speed = 24Mhz
//PCLK Speed = 72Mhz

//----[Init]----//
//SET_Device_Addr = 0x54
//Set_Device_Format = FORMAT_16_8
	{0x3100, 0x03},
	{0x3101, 0x80},
	{0x3102, 0x09},
	{0x3104, 0x03},
	{0x3105, 0x03},
	{0x3106, 0x05},
	{0x3107, 0x40},
	{0x3108, 0x00},
	{0x3109, 0x02},
	{0x310A, 0x04},
	{0x310B, 0x00},
	{0x310C, 0x00},
	{0x3111, 0x56},
	{0x3113, 0x66},
	{0x3118, 0xA7},
	{0x3119, 0xA7},
	{0x311A, 0xA7},
	{0x311B, 0x1F},
	{0x303f, 0x0e},
	{0x3041, 0x04},
	{0x3051, 0xf4},
	{0x320A, 0x5A},
	{0x3250, 0x80},
	{0x3251, 0x01},
	{0x3252, 0x38},
	{0x3253, 0xA8},
	{0x3254, 0x01},
	{0x3255, 0x00},
	{0x3256, 0x8C},
	{0x3257, 0x70},
	{0x329B, 0x00},
	{0x32A1, 0x00},
	{0x32A2, 0xd8},
	{0x32A3, 0x01},
	{0x32A4, 0x5d},
	{0x32A5, 0x01},
	{0x32A6, 0x0c},
	{0x32A7, 0x01},
	{0x32A8, 0xa8},
	{0x3210, 0x32},
	{0x3211, 0x2a},
	{0x3212, 0x30},
	{0x3213, 0x32},
	{0x3214, 0x2e},
	{0x3215, 0x2e},
	{0x3216, 0x2e},
	{0x3217, 0x2e},
	{0x3218, 0x2c},
	{0x3219, 0x2e},
	{0x321A, 0x2a},
	{0x321B, 0x2a},
	{0x321C, 0x2e},
	{0x321D, 0x2e},
	{0x321E, 0x28},
	{0x321F, 0x2c},
	{0x3220, 0x01},
	{0x3221, 0x48},
	{0x3222, 0x01},
	{0x3223, 0x48},
	{0x3224, 0x01},
	{0x3225, 0x48},
	{0x3226, 0x01},
	{0x3227, 0x48},
	{0x3228, 0x00},
	{0x3229, 0xa4},
	{0x322A, 0x00},
	{0x322B, 0xa4},
	{0x322C, 0x00},
	{0x322D, 0xa4},
	{0x322E, 0x00},
	{0x322F, 0xa4},
	{0x3243, 0xc2},
	{0x3270, 0x10},
	{0x3271, 0x1B},
	{0x3272, 0x26},
	{0x3273, 0x3E},
	{0x3274, 0x4F},
	{0x3275, 0x5E},
	{0x3276, 0x78},
	{0x3277, 0x8F},
	{0x3278, 0xA3},
	{0x3279, 0xB4},
	{0x327A, 0xD2},
	{0x327B, 0xE3},
	{0x327C, 0xF0},
	{0x327D, 0xF7},
	{0x327E, 0xFF},
	{0x33c2, 0xF0},
	{0x3060, 0x01},
	{0x3302, 0x00},
	{0x3303, 0x3E},
	{0x3304, 0x00},
	{0x3305, 0xC2},
	{0x3306, 0x00},
	{0x3307, 0x00},
	{0x3308, 0x07},
	{0x3309, 0xC4},
	{0x330A, 0x06},
	{0x330B, 0xED},
	{0x330C, 0x01},
	{0x330D, 0x4F},
	{0x330E, 0x01},
	{0x330F, 0x41},
	{0x3310, 0x06},
	{0x3311, 0xDD},
	{0x3312, 0x07},
	{0x3313, 0xE3},
	{0x3326, 0x14},
	{0x3327, 0x04},
	{0x3328, 0x04},
	{0x3329, 0x02},
	{0x332A, 0x02},
	{0x332B, 0x1D},
	{0x332C, 0x1D},
	{0x332D, 0x04},
	{0x332E, 0x1E},
	{0x332F, 0x1F},
	{0x3331, 0x0a},
	{0x3332, 0x40},
	{0x33C9, 0xD8},
	{0x33C0, 0x01},
	{0x333F, 0x07},
	{0x3360, 0x10},
	{0x3361, 0x18},
	{0x3362, 0x1f},
	{0x3363, 0xb3},
	{0x3368, 0xb0},
	{0x3369, 0xa0},
	{0x336A, 0x90},
	{0x336B, 0x80},
	{0x336C, 0x00},
	{0x3364, 0x00},
	{0x3365, 0x10},
	{0x3366, 0x06},
	{0x336d, 0x18},
	{0x336e, 0x18},
	{0x336f, 0x10},
	{0x3370, 0x10},
	{0x3371, 0x3F},
	{0x3372, 0x3F},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x20},
	{0x3376, 0x20},
	{0x3377, 0x28},
	{0x3378, 0x30},
	{0x32f6, 0x0C},
	{0x33A0, 0xE0},
	{0x33A1, 0x20},
	{0x33A2, 0x00},
	{0x33A3, 0x40},
	{0x33A4, 0x00},
};

struct NT_RegValue g_sNT99160_HD720[] = 
{
	//----[YUYV_1280x720_30.00Fps]----//
	{0x32BF, 0x60},
	{0x32C0, 0x5A}, 
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00}, 
	{0x32C4, 0x20},
	{0x32C5, 0x20}, 
	{0x32C6, 0x20}, 
	{0x32C7, 0x00}, 
	{0x32C8, 0xE0}, 
	{0x32C9, 0x5A}, 
	{0x32CA, 0x7A}, 
	{0x32CB, 0x7A},
	{0x32CC, 0x7A}, 
	{0x32CD, 0x7A}, 
	{0x32DB, 0x7B}, 
	{0x3241, 0x7A}, 
	{0x32F0, 0x00}, 
	{0x3200, 0x3E}, 
	{0x3201, 0x0F}, 
	{0x302A, 0x04}, 
	{0x302C, 0x07}, 
	{0x302D, 0x00}, 
	{0x302E, 0x00}, 
	{0x302F, 0x00}, 
	{0x3022, 0x24}, 
	{0x3023, 0x24}, 
	{0x3002, 0x00}, 
	{0x3003, 0x04}, 
	{0x3004, 0x00}, 
	{0x3005, 0x04}, 
	{0x3006, 0x05}, 
	{0x3007, 0x03}, 
	{0x3008, 0x02}, 
	{0x3009, 0xD3}, 
	{0x300A, 0x06}, 
	{0x300B, 0x48}, 
	{0x300C, 0x02}, 
	{0x300D, 0xEA}, 
	{0x300E, 0x05}, 
	{0x300F, 0x00}, 
	{0x3010, 0x02}, 
	{0x3011, 0xD0}, 
	{0x32B8, 0x3F}, 
	{0x32B9, 0x31}, 
	{0x32BB, 0x87}, 
	{0x32BC, 0x38}, 
	{0x32BD, 0x3C}, 
	{0x32BE, 0x34}, 
	{0x3201, 0x3F}, 
	{0x3109, 0x02}, 
	{0x310B, 0x00}, 
	{0x3530, 0xC0}, 
	{0x3021, 0x06}, 
	{0x3060, 0x01}
};


struct NT_RegValue g_sNT99160_SVGA[] = 
{
	//----[YUYV_800x600_38_Fps]----//
	{0x32BF, 0x60},
	{0x32C0, 0x55}, 
	{0x32C1, 0x54},
	{0x32C2, 0x54}, 
	{0x32C3, 0x00}, 
	{0x32C4, 0x20}, 
	{0x32C5, 0x20}, 
	{0x32C6, 0x20}, 
	{0x32C7, 0x40}, 
	{0x32C8, 0x17}, 
	{0x32C9, 0x54}, 
	{0x32CA, 0x74}, 
	{0x32CB, 0x74}, 
	{0x32CC, 0x74}, 
	{0x32CD, 0x75}, 
	{0x32DB, 0x81}, 
	{0x3241, 0x7B}, 
	{0x32E0, 0x03}, 
	{0x32E1, 0x20}, 
	{0x32E2, 0x02}, 
	{0x32E3, 0x58}, 
	{0x32E4, 0x00}, 
	{0x32E5, 0x33}, 
	{0x32E6, 0x00}, 
	{0x32E7, 0x33}, 
	{0x32E8, 0x01}, 
	{0x32F0, 0x00}, 
	{0x3200, 0x3E}, 
	{0x3201, 0x0F}, 
	{0x302A, 0x04}, 
	{0x302C, 0x07}, 
	{0x302D, 0x00}, 
	{0x302E, 0x00}, 
	{0x302F, 0x00}, 
	{0x3022, 0x24}, 
	{0x3023, 0x24}, 
	{0x3002, 0x00}, 
	{0x3003, 0xA4}, 
	{0x3004, 0x00}, 
	{0x3005, 0x04}, 
	{0x3006, 0x04}, 
	{0x3007, 0x63}, 
	{0x3008, 0x02}, 
	{0x3009, 0xD3}, 
	{0x300A, 0x05}, 
	{0x300B, 0x0A}, 
	{0x300C, 0x02}, 
	{0x300D, 0xE0}, 
	{0x300E, 0x03}, 
	{0x300F, 0xC0}, 
	{0x3010, 0x02}, 
	{0x3011, 0xD0}, 
	{0x32B8, 0x3F}, 
	{0x32B9, 0x31}, 
	{0x32BB, 0x87}, 
	{0x32BC, 0x38}, 
	{0x32BD, 0x3C}, 
	{0x32BE, 0x34}, 
	{0x3201, 0x7F}, 
	{0x3109, 0x02}, 
	{0x310B, 0x00}, 
	{0x3530, 0xC0}, 
	{0x3021, 0x06}, 
	{0x3060, 0x01},   
};

struct NT_RegValue g_sNT99160_VGA_38FPS[] = 
{
	//----[YUYV_640x480_38_Fps]----//
	{0x32BF, 0x60}, 
	{0x32C0, 0x55}, 
	{0x32C1, 0x54},
	{0x32C2, 0x54},
	{0x32C3, 0x00}, 
	{0x32C4, 0x20}, 
	{0x32C5, 0x20}, 
	{0x32C6, 0x20}, 
	{0x32C7, 0x40}, 
	{0x32C8, 0x17}, 
	{0x32C9, 0x54}, 
	{0x32CA, 0x74}, 
	{0x32CB, 0x74}, 
	{0x32CC, 0x74}, 
	{0x32CD, 0x75}, 
	{0x32DB, 0x81}, 
	{0x3241, 0x7B}, 
	{0x32E0, 0x02}, 
	{0x32E1, 0x80}, 
	{0x32E2, 0x01}, 
	{0x32E3, 0xE0}, 
	{0x32E4, 0x00}, 
	{0x32E5, 0x80},
	{0x32E6, 0x00}, 
	{0x32E7, 0x80}, 
	{0x32E8, 0x01}, 
	{0x32F0, 0x00}, 
	{0x3200, 0x3E}, 
	{0x3201, 0x0F}, 
	{0x302A, 0x04}, 
	{0x302C, 0x07}, 
	{0x302D, 0x00}, 
	{0x302E, 0x00}, 
	{0x302F, 0x00}, 
	{0x3022, 0x24}, 
	{0x3023, 0x24}, 
	{0x3002, 0x00}, 
	{0x3003, 0xA4}, 
	{0x3004, 0x00}, 
	{0x3005, 0x04}, 
	{0x3006, 0x04}, 
	{0x3007, 0x63}, 
	{0x3008, 0x02}, 
	{0x3009, 0xD3}, 
	{0x300A, 0x05}, 
	{0x300B, 0x0A}, 
	{0x300C, 0x02}, 
	{0x300D, 0xE0}, 
	{0x300E, 0x03}, 
	{0x300F, 0xC0}, 
	{0x3010, 0x02}, 
	{0x3011, 0xD0}, 
	{0x32B8, 0x3F},
	{0x32B9, 0x31}, 
	{0x32BB, 0x87}, 
	{0x32BC, 0x38}, 
	{0x32BD, 0x3C}, 
	{0x32BE, 0x34}, 
	{0x3201, 0x7F}, 
	{0x3109, 0x02}, 
	{0x310B, 0x00}, 
	{0x3530, 0xC0}, 
	{0x3021, 0x06}, 
	{0x3060, 0x01}
};

struct NT_RegValue g_sNT99160_50Hz[] = 
{
	//[50Hz]  
	0, 0
};

struct NT_RegValue g_sNT99160_60Hz[] = 
{
	//[60Hz]       
	0, 0	
};


struct NT_RegTable g_NT99160_InitTable[] =
{
	{g_sNT99160_Init,_REG_TABLE_SIZE(g_sNT99160_Init)},
	{g_sNT99160_VGA_38FPS, _REG_TABLE_SIZE(g_sNT99160_VGA_38FPS)},	
	{g_sNT99160_SVGA,_REG_TABLE_SIZE(g_sNT99160_SVGA)},
	{g_sNT99160_HD720,_REG_TABLE_SIZE(g_sNT99160_HD720)},		
	
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
	0x54,		// NT99141 = 8
	0x54,		// NT99160 = 9
	0x54,		// NT99161 = 10
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
/*
	The sensor should  be initialized in VGA in advance. Then other mode. Otherwise, the sensor should be initialzed fail if start from SVGA or HD mode. 
*/
VOID NT99160_Init(UINT32 nIndex, UINT32 u32Resolution)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 id0, id1;
	struct NT_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;	
	sysDelay(2);
	
	/* NT99160 sensor need reset without sensor clock */
	SnrPowerDown(FALSE); 	 	
	SnrReset();				
	
	sysDelay(2);
	u32TableSize = g_NT99160_InitTable[0].uTableSize;
	psRegValue = g_NT99160_InitTable[0].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
	if ( psRegValue == 0 )
		return;	


	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB13));
	outp32(REG_GPBFUN1, inp32(REG_GPBFUN1) & (~MF_GPB14));
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);												
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));						
	}
		
	id0 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_H);
	id1 = I2C_Read_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID,CHIP_VERSION_L);
	//static UINT8 I2C_Read_8bitSlaveAddr_16bitReg_8bitData(UINT8 uAddr, UINT16 uRegAddr)
	sysprintf("Detectd sensor id0=%0x id1=%02x\n",id0, id1);
	
	u32TableSize = g_NT99160_InitTable[u32Resolution].uTableSize;
	psRegValue = g_NT99160_InitTable[u32Resolution].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));						
	}

	DrvI2C_Close();	
}


UINT32 Smpl_NT99160(void)
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
	NT99160_Init(9, REG_VALUE_VGA);			
	pVin->EnableInt(eVIDEOIN_VINT);
	pVin->InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt							
	pVin->SetPacketFrameBufferControl(FALSE, FALSE);		
												
	pVin->SetDataFormatAndOrder(eVIDEOIN_IN_VYUY, 				//NT99160
								eVIDEOIN_IN_YUV422	,	//Intput format
								eVIDEOIN_OUT_YUV422);		//Output format for packet
	pVin->SetCropWinStartAddr(0,							//Horizontal start position	
							0);						//Useless
	
	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
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
	
#ifdef _MACRO_BLOCK_														
	pVin->SetPlanarFormat(eVIDEOIN_MACRO_PLANAR_YUV420);		// Planar YUV422/420/macro 	
	pVin->SetBaseStartAddress(eVIDEOIN_PLANAR,			
							2, 							//Planar buffer V addrress
							(UINT32)u8PlanarFrameBuffer0+OPT_ENCODE_WIDTH*OPT_ENCODE_HEIGHT);

#endif		
	pVin->SetPipeEnable(TRUE, 								// Engine enable?
						eVIDEOIN_BOTH_PIPE_ENABLE);		// which packet was enabled. 											
										
	pVin->SetShadowRegister();				
	sysSetLocalInterrupt(ENABLE_IRQ);		
																				
	return Successful;			
}	
