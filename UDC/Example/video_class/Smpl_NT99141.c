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
#define REG_VALUE_VGA		1	//640X480
#define REG_VALUE_SVGA	2	//800X600
#define REG_VALUE_HD720	3	//1280X720



#define DrvVideoIn_nt99140	1
#define CHIP_VERSION_H		0x3000
#define CHIP_VERSION_L		0x3001
#define NT99140_CHIP_ID	0x14

struct NT_RegTable{
	struct NT_RegValue *sRegTable;
	UINT16 uTableSize;
};

struct NT_RegValue g_sNT99141_Init[] = 
{
	//[Inti]
	{0x3109, 0x04},
	{0x3040, 0x04},
	{0x3041, 0x02},
	{0x3042, 0xFF},
	{0x3043, 0x08},
	{0x3052, 0xE0},
	{0x305F, 0x33},
	{0x3100, 0x07},
	{0x3106, 0x03},

	{0x3108, 0x00},
	{0x3110, 0x22},
	{0x3111, 0x57},
	{0x3112, 0x22},
	{0x3113, 0x55},
	{0x3114, 0x05},
	{0x3135, 0x00},
	{0x32F0, 0x01},
	
	{0x306a,0x01}, //

	{0x3290, 0x01}, // Initial AWB Gain
	{0x3291, 0x80},
	{0x3296, 0x01},
	{0x3297, 0x73},

	{0x3250, 0x80}, // CA Ratio
	{0x3251, 0x03},
	{0x3252, 0xFF},
	{0x3253, 0x00},
	{0x3254, 0x03},
	{0x3255, 0xFF},
	{0x3256, 0x00},
	{0x3257, 0x50},

	{0x3270, 0x00}, // Gamma
	{0x3271, 0x0C},
	{0x3272, 0x18},
	{0x3273, 0x32},
	{0x3274, 0x44},
	{0x3275, 0x54},
	{0x3276, 0x70},
	{0x3277, 0x88},
	{0x3278, 0x9D},
	{0x3279, 0xB0},
	{0x327A, 0xCF},
	{0x327B, 0xE2},
	{0x327C, 0xEF},
	{0x327D, 0xF7},
	{0x327E, 0xFF},

	{0x3302, 0x00}, // Color Correction
	{0x3303, 0x40},
	{0x3304, 0x00},
	{0x3305, 0x96},
	{0x3306, 0x00},
	{0x3307, 0x29},
	{0x3308, 0x07},
	{0x3309, 0xBA},
	{0x330A, 0x06},
	{0x330B, 0xF5},
	{0x330C, 0x01},
	{0x330D, 0x51},
	{0x330E, 0x01},
	{0x330F, 0x30},
	{0x3310, 0x07},
	{0x3311, 0x16},
	{0x3312, 0x07},
	{0x3313, 0xBA},

	{0x3326, 0x02}, // EExt
	{0x32F6, 0x0F},
	{0x32F9, 0x42},
	{0x32FA, 0x24},
	{0x3325, 0x4A},
	{0x3330, 0x00},
	{0x3331, 0x0A},
	{0x3332, 0xFF},
	{0x3338, 0x30},
	{0x3339, 0x84},
	{0x333A, 0x48},
	{0x333F, 0x07},

	{0x3360, 0x10}, // Auto Function
	{0x3361, 0x18},
	{0x3362, 0x1f},
	{0x3363, 0x37},
	{0x3364, 0x80},
	{0x3365, 0x80},
	{0x3366, 0x68},
	{0x3367, 0x60},
	{0x3368, 0x30},
	{0x3369, 0x28},
	{0x336A, 0x20},
	{0x336B, 0x10},
	{0x336C, 0x00},
	{0x336D, 0x20},
	{0x336E, 0x1C},
	{0x336F, 0x18},
	{0x3370, 0x10},
	{0x3371, 0x38},
	{0x3372, 0x3C},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x338A, 0x34},
	{0x338B, 0x7F},
	{0x338C, 0x10},
	{0x338D, 0x23},
	{0x338E, 0x7F},
	{0x338F, 0x14},
	{0x3375, 0x0A}, 
	{0x3376, 0x0C}, 
	{0x3377, 0x10}, 
	{0x3378, 0x14}, 

	{0x3012, 0x02},
	{0x3013, 0xD0},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_HD720[] = 
{
	//[YUYV_1280x720_30.00_30.04_Fps]  MCLK 24 M  PCLK 74M Fix 30.01_Fps
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0xDF},
	{0x32C9, 0x5A},
	{0x32CA, 0x6D},
	{0x32CB, 0x6D},
	{0x32CC, 0x79},
	{0x32CD, 0x79},
	{0x32DB, 0x7B},
	{0x32E0, 0x05},
	{0x32E1, 0x00},
	{0x32E2, 0x02},
	{0x32E3, 0xD0},
	{0x32E4, 0x00},
	{0x32E5, 0x00},
	{0x32E6, 0x00},
	{0x32E7, 0x00},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x24},
	{0x3029, 0x20},
	{0x302A, 0x04},
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
	{0x300B, 0x7C},
	{0x300C, 0x02},
	{0x300D, 0xE6},
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
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};


struct NT_RegValue g_sNT99141_SVGA[] = 
{
	//[YUYV_800x600_30.00_30.04_Fps]  MCLK 24 M  PCLK 74M Fix 30.01_Fps
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0xDF},
	{0x32C9, 0x5A},
	{0x32CA, 0x6D},
	{0x32CB, 0x6D},
	{0x32CC, 0x79},
	{0x32CD, 0x79},
	{0x32DB, 0x7B},
	{0x32E0, 0x03},
	{0x32E1, 0x20},
	{0x32E2, 0x02},
	{0x32E3, 0x58},
	{0x32E4, 0x00},
	{0x32E5, 0x33},
	{0x32E6, 0x00},
	{0x32E7, 0x33},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x24},
	{0x3029, 0x20},
	{0x302A, 0x04},
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
	{0x300A, 0x06},
	{0x300B, 0x7C},
	{0x300C, 0x02},
	{0x300D, 0xE6},
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
	{0x3021, 0x06},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_VGA[] = 
{
	//[YUYV_640x480 24M MCLK, 60MHz PCLK 30F/S]
	{0x32BF, 0x60},
	{0x32C0, 0x61},
	{0x32C1, 0x61},
	{0x32C2, 0x61},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0x76},
	{0x32C9, 0x61},
	{0x32CA, 0x74},
	{0x32CB, 0x74},
	{0x32CC, 0x80},
	{0x32CD, 0x80},
	{0x32DB, 0x6D},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x00},
	{0x32E6, 0x00},
	{0x32E7, 0x00},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x07},
	{0x3029, 0x00},
	{0x302A, 0x08},
	{0x3022, 0x24},
	{0x3023, 0x24},
	{0x3002, 0x01},
	{0x3003, 0x44},
	{0x3004, 0x00},
	{0x3005, 0x7C},
	{0x3006, 0x03},
	{0x3007, 0xC3},
	{0x3008, 0x02},
	{0x3009, 0x5B},
	{0x300A, 0x03},
	{0x300B, 0xFC},
	{0x300C, 0x01},
	{0x300D, 0xFF},
	{0x300E, 0x02},
	{0x300F, 0x80},
	{0x3010, 0x01},
	{0x3011, 0xE0},
	{0x32B8, 0x3F},
	{0x32B9, 0x31},
	{0x32BB, 0x87},
	{0x32BC, 0x38},
	{0x32BD, 0x3C},
	{0x32BE, 0x34},
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_VGA_30FPS[] = 
{
	 //[YUYV_640x480_30.00_30.01_Fps]  MCLK 24 M  PCLK 60M   Fix 30.01_Fps
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x27},
	{0x32C5, 0x13},
	{0x32C6, 0x1F},
	{0x32C7, 0x00},
	{0x32C8, 0xE0},
	{0x32C9, 0x5A},
	{0x32CA, 0x6D},
	{0x32CB, 0x6D},
	{0x32CC, 0x79},
	{0x32CD, 0x79},
	{0x32DB, 0x7B},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x80},
	{0x32E6, 0x00},
	{0x32E7, 0x80},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x09},
	{0x3029, 0x00},
	{0x302A, 0x04},
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
	{0x300B, 0x3C},
	{0x300C, 0x02},
	{0x300D, 0xEA},
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
	{0x3021, 0x06},
	{0x3060, 0x01},
};

struct NT_RegValue g_sNT99141_50Hz[] = 
{
	//[50Hz]  
	{0x32BF, 0x60}, 
	{0x32C0, 0x5A}, 
	{0x32C1, 0x5A}, 
	{0x32C2, 0x5A}, 
	{0x32C3, 0x00}, 
	{0x32C4, 0x27}, 
	{0x32C5, 0x13}, 
	{0x32C6, 0x1F}, 
	{0x32C7, 0x00}, 
	{0x32C8, 0xDF}, 
	{0x32C9, 0x5A}, 
	{0x32CA, 0x6D}, 
	{0x32CB, 0x6D}, 
	{0x32CC, 0x79}, 
	{0x32CD, 0x79}, 
	{0x32DB, 0x7B}, 
};

struct NT_RegValue g_sNT99141_60Hz[] = 
{
	//[60Hz]       
	{0x32BF, 0x60}, 
	{0x32C0, 0x60}, 
	{0x32C1, 0x5F}, 
	{0x32C2, 0x5F}, 
	{0x32C3, 0x00}, 
	{0x32C4, 0x27}, 
	{0x32C5, 0x13}, 
	{0x32C6, 0x1F}, 
	{0x32C7, 0x00}, 
	{0x32C8, 0xBA}, 
	{0x32C9, 0x5F}, 
	{0x32CA, 0x72}, 
	{0x32CB, 0x72}, 
	{0x32CC, 0x7E}, 
	{0x32CD, 0x7F}, 
	{0x32DB, 0x77}, 
};


struct NT_RegTable g_NT99141_InitTable[] =
{
	{g_sNT99141_Init,_REG_TABLE_SIZE(g_sNT99141_Init)},
	{g_sNT99141_VGA_30FPS, _REG_TABLE_SIZE(g_sNT99141_VGA_30FPS)},	
	{g_sNT99141_SVGA,_REG_TABLE_SIZE(g_sNT99141_SVGA)},
	{g_sNT99141_HD720,_REG_TABLE_SIZE(g_sNT99141_HD720)},		
	
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

VOID NT99141_Init(UINT32 nIndex, UINT32 u32Resolution)
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
	
	//NT99141 need to be reset.	
	SnrPowerDown(FALSE); 	 	
	SnrReset();				
	
	sysDelay(2);
	u32TableSize = g_NT99141_InitTable[0].uTableSize;
	psRegValue = g_NT99141_InitTable[0].sRegTable;
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
	
	u32TableSize = g_NT99141_InitTable[u32Resolution].uTableSize;
	psRegValue = g_NT99141_InitTable[u32Resolution].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{		
		I2C_Write_8bitSlaveAddr_16bitReg_8bitData(u8DeviceID, (psRegValue->u16RegAddr), (psRegValue->u8Value));						
	}

	DrvI2C_Close();	
}

UINT32 Smpl_NT99141(void)
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
	NT99141_Init(8, REG_VALUE_VGA);			
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
	
	pVin->SetStandardCCIR656(TRUE);						//standard CCIR656 mode		
 	pVin->SetSensorPolarity(FALSE,							
						FALSE,						
						FALSE);							
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
