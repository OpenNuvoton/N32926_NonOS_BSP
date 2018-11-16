#include "w55fa92_gnand.h"

typedef void (*PFN_MSCD_CDROM_CALLBACK)(PUINT32 pu32address, UINT32 u32Offset, UINT32 u32LengthInByte);

#define MSC_SD_DISABLE		0x00	/* Disable SD port */
#define MSC_SD_PORT0		0x01	/* SD port 0 (Single partition) */
#define MSC_SD_PORT1		0x02	/* SD port 1 (Single partition) */
#define MSC_SD_PORT2		0x04	/* SD port 2 (Single partition) */
#define MSC_SD_MP_PORT0		0x11	/* SD port 0 (Multiple partition) */
#define MSC_SD_MP_PORT1		0x22	/* SD port 1 (Multiple partition) */
#define MSC_SD_MP_PORT2		0x44	/* SD port 2 (Multiple partition) */

/* For MSC card detect and write protect */
#define MSC_SD_GPIO_PORTA	0		
#define MSC_SD_GPIO_PORTB	1
#define MSC_SD_GPIO_PORTC	2
#define MSC_SD_GPIO_PORTD	3
#define MSC_SD_GPIO_PORTE	4
#define MSC_SD_GPIO_PORTG	5
#define MSC_SD_GPIO_PORTH	6


#define MSC_NAND_DISABLE	0x00	/* Disable NAND */
#define MSC_NAND_CS0		0x01	/* NAND CS0 */
#define MSC_NAND_CS1		0x02	/* NAND CS1 */
#define MSC_NAND_CS2		0x04	/* NAND CS2 */

#define MSC_RAM_DISABLE		0x00
#define MSC_RAM_ENABLE		0x01

#define MSC_CDROM_DISABLE	0x00
#define MSC_CDROM_ENABLE	0x01


/* for RAM disk mass storage */
/* flash format */
#define	MSC_RAMDISK_1M			1
#define	MSC_RAMDISK_4M			2
#define	MSC_RAMDISK_8M			3
#define	MSC_RAMDISK_16M			4
#define	MSC_RAMDISK_32M			5
#define	MSC_RAMDISK_64M			6
#define	MSC_RAMDISK_128M		7


/* extern functions */
VOID mscdInit(void);
/* The callback function is Plug detection for mscdMassEvent (Retrun vale - TRUE:Run mscdMassEvent;FALSE:Exit mscdMassEvent) */
void mscdMassEvent(PFN_USBD_EXIT_CALLBACK* callback_func);
void mscdHighSpeedInit(void);
void mscdFullSpeedInit(void);
/* Export only one CS NAND (default is CS 0;controlled by mscdNandEnable) or only one port SD (default is port 0;controlled by mscdSdEnable) */
UINT8 mscdFlashInit(NDISK_T *pDisk, INT SDsector);
/* Export only one port SD (default is port 0;controlled by mscdSdEnable)*/
UINT8 mscdFlashInitNAND(NDISK_T *pDisk,NDISK_T *pDisk1,NDISK_T *pDisk2, INT SDsector);
/* Export all export (controlled by the linking library and API-mscdSdEnable&mscdNandEnable) */
UINT8 mscdFlashInitExtend(NDISK_T *pDisk,NDISK_T *pDisk1,NDISK_T *pDisk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize);
/* Export only one CS NAND or only one port SD */
UINT8 mscdFlashInitCDROM(NDISK_T *pDisk, INT SDsector, PFN_MSCD_CDROM_CALLBACK pfnCallBack ,INT CdromSizeInByte);
UINT8 mscdFlashInitExtendCDROM(NDISK_T *pDisk,NDISK_T *pDisk1,NDISK_T *pDisk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize, PFN_MSCD_CDROM_CALLBACK pfnCallBack ,INT CdromSizeInByte);
VOID mscdSdEnable(UINT32 u32Enable);
VOID mscdNandEnable(UINT32 u32Enable);
/* Enable and Set user define Write Protect Pin (default is enable and use the default pin - Port 0 (GPA0) Only) */
VOID mscdSdUserWriteProtectPin(UINT32 u32SdPort, BOOL bEnable, UINT32 u32GpioPort, UINT32 u32GpioPin);
/* Enable and Set user define Card Detect Pin (default is enable and use the default pin - Port 0(GPA1) & 2(GPE11) Only) */
VOID mscdSdUserCardDetectPin(UINT32 u32SdPort, BOOL bEnable, UINT32 u32GpioPort, UINT32 u32GpioPin);
VOID mscdDeinit(void);
VOID mscdBlcokModeEnable(BOOL bEnable);