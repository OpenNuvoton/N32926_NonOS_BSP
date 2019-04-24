/****************************************************************************
*                                                                           *
* Copyright (c) 2009 Nuvoton Tech. Corp. All rights reserved.               *
*                                                                           *
*****************************************************************************/
#include "w55fa92_gnand.h"

#define KERNEL_PATH_SD 	"x:\\conprog.bin"
#define MOVIE_PATH_SD 		"x:\\movie.avi"
#define MP3_PATH_SD 		"x:\\audio.mp3"

#define FIRMWARE_PATH_SD 	"x:\\SDBoot.bin"

#define KERNEL_PATH 		"c:\\conprog.bin"
#define MOVIE_PATH 		"c:\\movie.avi"
#define MP3_PATH 			"c:\\audio.mp3"

#if defined(__ENABLE_SD_CARD_0__)|| defined(__ENABLE_SD_CARD_1__)||defined(__ENABLE_SD_CARD_2__)
#define VOLUME_PATH		"x:\\volume.cfg"
#define SATURATION_PATH	"x:\\saturation.cfg"
#else
#define VOLUME_PATH		"c:\\volume.cfg"
#define SATURATION_PATH	"c:\\saturation.cfg"
#endif		

#define CP_SIZE 		(16 * 1024)
#define PANEL_BPP		2
#define FB_ADDR		0x500000

/* Start for option for VPOST frame buffer */
#if defined(__TV__)
	#ifdef __TV_QVGA__ 
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT		240
	#else
	#define PANEL_WIDTH		640
	#define PANEL_HEIGHT		480
	#endif
#elif defined( __LCM_800x600__) 
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT		600
#elif defined( __LCM_480x272__)
	#define PANEL_WIDTH		480
	#define PANEL_HEIGHT		272
#elif defined( __LCM_800x480__)
	#define PANEL_WIDTH		800
	#define PANEL_HEIGHT		480
#elif defined( __LCM_VGA__)
	#define PANEL_WIDTH		640
	#define PANEL_HEIGHT		480	
#elif defined( __LCM_QVGA__)
	#define PANEL_WIDTH		320
	#define PANEL_HEIGHT		240
#elif defined( __LCM_128x64__)
	#define PANEL_WIDTH		128
	#define PANEL_HEIGHT		64	
#else 	
	#define PANEL_WIDTH		480
	#define PANEL_HEIGHT		272
#endif

/* Defined For Key Matrix And Low Battery Option */
#if defined(__NVT_DEV_DEMO_SD__)
	#define B_KEY				(1)
	#define A_KEY				(2)
	#define LEFT_KEY			(0x8)
	#define RIGHT_KEY			(0x4)
	#define UP_KEY				(0x10)
	#define DOWN_KEY			(0x20)

	//#define HOME_KEY			B_KEY		//Escape
	//#define ENTER_KEY		A_KEY
	#define HOME_KEY			A_KEY		//Escape
	#define ENTER_KEY			B_KEY
	//#define MASS_STORAGE		(HOME_KEY+LEFT_KEY)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE		 32		/* MB unit */
	#define SD1_1_SIZE	 		128  	/* MB unit */ 
	#define NAND2_1_SIZE		1024 	/* MB unit */

	#define KEY_ADC_CHANNEL	2
	
	#define __HAVE_VPOST__
	#define __AVI_PLAYBACK__
	/* USB Mass Storage*/
	#define __SD_ONLY__				
	#define __SD__
#endif
#if defined(__NVT_DEV_DEMO_NAND__)
	#define B_KEY				(1)
	#define A_KEY				(2)
	#define LEFT_KEY			(0x8)
	#define RIGHT_KEY			(0x4)
	#define UP_KEY				(0x10)
	#define DOWN_KEY			(0x20)

	//#define HOME_KEY			B_KEY		//Escape
	//#define ENTER_KEY		A_KEY
	#define HOME_KEY			A_KEY		//Escape
	#define ENTER_KEY			B_KEY
	//#define MASS_STORAGE		(HOME_KEY+LEFT_KEY)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE		 32		/* MB unit */
	#define SD1_1_SIZE	 		128  	/* MB unit */ 
	#define NAND2_1_SIZE		1024 	/* MB unit */

	#define KEY_ADC_CHANNEL	2
	
	#define __HAVE_VPOST__
	#define __AVI_PLAYBACK__
	/* USB Mass Storage*/				
	#define __NAND__
#endif
#if defined(__PS_DEMO_SD__)	/*Clone from __NVT_DEV_DEMO_SD__ */
	#define B_KEY				(1)
	#define A_KEY				(2)
	#define HOME_KEY			A_KEY		//Escape
	#define ENTER_KEY			B_KEY
	#define LEFT_KEY			(0x8)
	#define RIGHT_KEY			(0x4)
	#define UP_KEY				(0x10)
	#define DOWN_KEY			(0x20)

	#define POWER_KEY			0x80000000
	#define MASS_STORAGE		(POWER_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE		 32		/* MB unit */
	#define SD1_1_SIZE	 		128  	/* MB unit */ 
	#define NAND2_1_SIZE		1024 	/* MB unit */

	#define KEY_ADC_CHANNEL	2
	
	/* USB Mass Storage*/
	#define __SD_ONLY__				
	#define __SD__
#endif
#if defined(__PS_DEMO_NAND__)
	#define B_KEY				(1)
	#define A_KEY				(2)
	#define HOME_KEY			A_KEY		//Escape
	#define ENTER_KEY			B_KEY
	#define LEFT_KEY			(0x8)
	#define RIGHT_KEY			(0x4)
	#define UP_KEY				(0x10)
	#define DOWN_KEY			(0x20)
	
	#define POWER_KEY			0x80000000
	#define MASS_STORAGE		(POWER_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE		 32		/* MB unit */
	#define SD1_1_SIZE	 		128  	/* MB unit */ 
	#define NAND2_1_SIZE		1024 	/* MB unit */

	#define KEY_ADC_CHANNEL	2
	
	/* USB Mass Storage*/				
	#define __NAND__
#endif

#if defined(__NVT_DEV_DEMO_NAND_BMILK__)
	#define B_KEY				(1)
	#define A_KEY				(2)
	#define LEFT_KEY			(0x8)
	#define RIGHT_KEY			(0x4)
	#define UP_KEY				(0x10)
	#define DOWN_KEY			(0x20)

	//#define HOME_KEY			B_KEY		//Escape
	//#define ENTER_KEY		A_KEY
	#define HOME_KEY			A_KEY		//Escape
	#define ENTER_KEY			B_KEY
	//#define MASS_STORAGE		(HOME_KEY+LEFT_KEY)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE		 32		/* MB unit */
	#define SD1_1_SIZE	 		128  	/* MB unit */ 
	#define NAND2_1_SIZE		1024 	/* MB unit */

	#define KEY_ADC_CHANNEL	2
	
	#define __HAVE_VPOST__
	/* USB Mass Storage*/				
	#define __NAND__
#endif
#if defined(__NVT_DEV_DEMO_SD_BMILK__)
	#define B_KEY				(1)
	#define A_KEY				(2)
	#define LEFT_KEY			(0x8)
	#define RIGHT_KEY			(0x4)
	#define UP_KEY				(0x10)
	#define DOWN_KEY			(0x20)

	//#define HOME_KEY			B_KEY		//Escape
	//#define ENTER_KEY		A_KEY
	#define HOME_KEY			A_KEY		//Escape
	#define ENTER_KEY			B_KEY
	//#define MASS_STORAGE		(HOME_KEY+LEFT_KEY)
	#define MASS_STORAGE		(UP_KEY+DOWN_KEY)
	#define LOW_BATTERY_LEVEL	(3.5)
	/* NAND1-1 Size */
	#define NAND1_1_SIZE		 32		/* MB unit */
	#define SD1_1_SIZE	 		128  	/* MB unit */ 
	#define NAND2_1_SIZE		1024 	/* MB unit */

	#define KEY_ADC_CHANNEL	2
	
	#define __HAVE_VPOST__	
	/* USB Mass Storage*/
	#define __SD_ONLY__				
	#define __SD__
#endif

/* End for option for VPOST frame buffer */

#define PANEL_BPP		2
#define FB_ADDR		0x500000



#ifdef __DEBUG__
#define DBG_PRINTF		sysprintf
#else
#define DBG_PRINTF(...)		
#endif
 


typedef struct{
	void (*backlight_init)(void);
	void (*backlight_enable)(void);
	void (*backlight_disable)(void);	
	void (*lcmpower_init)(void);
	void (*lcmpower_enable)(void);
	void (*lcmpower_disable)(void);
	void (*spkpower_init)(void);
	void (*spkpower_enable)(void);
	void (*spkpower_disable)(void);
	void (*earphone_init)(void);
	BOOL (*earphone_detect)(void);
	void (*mute_init)(void);
	void (*mute_enable)(void);
	void (*mute_disable)(void);
}BOARD_S;


UINT32 NVT_LoadKernelFromSD(BOARD_S* pS_board, 
							UINT32 g_ibr_boot_sd_port,
							unsigned char* pkBuf);
UINT32 NVT_LoadKernelFromNAND(BOARD_S* ps_board, 
							UINT32 g_ibr_boot_sd_port,
							unsigned char* pkBuf);							
INT32 register_board(BOARD_S* ps_board);	
void initVPostShowLogo(BOARD_S* ps_board);						

INT32 kpi_read(UINT32 u32Channel);
void mass(NDISK_T *disk0, NDISK_T *disk1, NDISK_T *disk2, INT SDsector0,INT SDsector1,INT SDsector2, INT RamSize);
void playAnimation(BOARD_S* ps_board, int kfd, char* pcString);
void aviSetPlayVolume(int vol);
void loadKernelCont( int fd, int offset, unsigned char* pkBuf);


/* some API not defined in header file */
void spuSetVolume(UINT16 u16CHRVolume, UINT16 u16CHLVolume);
VOID spuSetDacSlaveMode(void);
