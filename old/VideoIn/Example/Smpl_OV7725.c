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


#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct OV_RegValue))

static struct OV_RegValue g_sOV7725_VGA_RegValue[]=
{//OV7725		

    {0x12, 0x80},  
#ifdef   OV7725_CCIR656  
    {0x12, 0x20}, 
    {0x15, 0x01}, /* Color Range from 0 ~240 */ 
#else
  {0x12, 0x00}, 	
  {0x15, 0x00}, /* full Range */ 	
#endif    
    {0x3d, 0x03}, 
    {0x17, 0x22},  
   // {0x17, 0x26},  
    {0x18, 0xa4},  /* Hsize = 10100100+ 00 */
    {0x19, 0x07}, 
    
     {0x1a, 0xf0},  /* VSIZE + 0x32[2] =0x1e0*/
    // {0x1a, 0xf2},     /* VSIZE + 0x32[2] = 0x1e4*/
    
     {0x32, 0x02},  
  
     {0x29, 0xa0},   /* Horizontal data output size + 0x29[1:0]*/   
   	
    
    {0x2c, 0xf0},   
//    {0x2a, 0x00},  /* default */
    {0x2a, 0x02},  /*  useless */
     
      //{0x33, 0x08},  /*  useless */
      //{0x34, 0x08},  /*  useless */
    
    {0x11, 0x03},//00/01/03/07 for 60/30/15/7.5fps         
    {0x42, 0x7f},   {0x4d, 0x09},  {0x63, 0xe0},  {0x64, 0xff},   {0x65, 0x20},  {0x66, 0x00},  {0x67, 0x48}, {0x13, 0xf0},      
    {0x0d, 0x41},  //0x51/0x61/0x71 for different AEC/AGC window   
    {0x0f, 0xc5},  {0x14, 0x11},//0x81   
    {0x22, 0x7f},//ff/7f/3f/1f for 60/30/15/7.5fps   
    {0x23, 0x03},//01/03/07/0f for 60/30/15/7.5fps   
    {0x24, 0x50},//0x80   
    {0x25, 0x30},//5a   
    {0x26, 0xa1},//c1   
    {0x2b, 0x00},//ff   
    {0x6b, 0xaa},  {0x13, 0xff},  {0x90, 0x05},  {0x91, 0x01},  {0x92, 0x03},  {0x93, 0x00}, {0x94, 0xb0},  {0x95, 0x9d},  
    {0x96, 0x13},  {0x97, 0x16},  {0x98, 0x7b},  {0x99, 0x91},  {0x9a, 0x1e},  {0x9b, 0x00},  {0x9c, 0x25},  {0x9e, 0x81},  
    {0xa6, 0x06},  
  
//modified saturation initialization value by pw 2008-03-04   
    {0xa7, 0x65},  {0xa8, 0x65},  {0x7e, 0x0c},  {0x7f, 0x16},  {0x80, 0x2a},  {0x81, 0x4e},  {0x82, 0x61},  {0x83, 0x6f},  
    {0x84, 0x7b},  {0x85, 0x86},  {0x86, 0x8e},  {0x87, 0x97},  {0x88, 0xa4},  {0x89, 0xaf},  {0x8a, 0xc5},  {0x8b, 0xd7},  
    {0x8c, 0xe8},   {0x8d, 0x20},  
    
#if 1   
    {0x34, 0x00},  {0x33, 0x40},//0x66/0x99   
    {0x22, 0x99},  {0x23, 0x03},  {0x4a, 0x10},  {0x49, 0x10},  {0x4b, 0x14},  {0x4c, 0x17},  {0x46, 0x05},  
    {0x0e, 0x65},  
#endif       
     {0x15, 1},  
    {0x69, 0x5d},  {0x0c, 0x00},  {0x33, 0x3f},//0x66/0x99   
    {0x0e, 0x65},  
    /* Remark by SW */
    /*		
    sharpness strength modified by pw 2008-03-05   
    regvalue = gpio_sccb_read{I2C_OV7725, 0xac},  
    {0xac, 0xdfRvalue},  
    */
    {0x8f, 0x04},  	
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
VOID OV7725_Init(UINT32 nIndex)
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
		
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
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
/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_OV7725_VGA(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1, UINT8* pu8FrameBuffer2)
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



