/*************************************************************************
 * Nuvoton Electronics Corporation confidential
 *
 * Copyright (c) 2008 by Nuvoton Electronics Corporation
 * All rights reserved
 *
 * FILENAME
 *     usbconfig.h
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     NUC900 USB Host driver header file 
 *
 * HISTORY
 *     2008.06.24       Created
 *
 * REMARK
 *     None
 **************************************************************************/
#ifndef  _USBCONFIG_H_
#define  _USBCONFIG_H_

#ifdef ECOS
#define INT_NUM_USBH  15
#endif

//#define B_VERSION
#define CB_VERSION

#define CONFIG_HAVE_OHCI
#define CONFIG_HAVE_EHCI



#define DEBUG
#define USB_VERBOSE_DEBUG		
#define CONFIG_USB_EHCI_SPLIT_ISO

//#define AUTO_SUSPEND					/* automatically suspend and resume */

#define EHCI_ISO_DELAY			8
#define OHCI_ISO_DELAY			8

#define EHCI_ITD_DELAY_FREE


/*
 *  Size of static memory buffers
 */
#define USB_MEMORY_POOL_SIZE   (4*1024*1024)
#define PERIODIC_FL_SIZE		4096
#define HCCA_SIZE				4096


/*
 *  Debug/Warning/Information to be printed on console or not
 */
#define USB_debug				sysprintf
#define USB_error				sysprintf
//#define USB_warning			sysprintf
//#define USB_info				sysprintf
//#define USB_error(...)
//#define USB_debug(...)	
#define USB_warning(...)	
#define USB_info(...)		

#ifdef ECOS
	#define sysprintf			diag_printf
#endif

#ifdef ECOS
#define DISABLE_USB_INT()       cyg_drv_interrupt_mask(INT_NUM_USBH)
#define ENABLE_USB_INT()		cyg_drv_interrupt_unmask(INT_NUM_USBH)
#else
#define DISABLE_USB_INT()       sysDisableInterrupt(IRQ_EHCI);sysDisableInterrupt(IRQ_OHCI)
#define ENABLE_USB_INT()		sysEnableInterrupt(IRQ_EHCI);sysEnableInterrupt(IRQ_OHCI)
#endif

/*
 * Porting
 */
#define le16_to_cpu(x)				(x)
#define cpu_to_le16(x)				(x)
#define cpu_to_le32(x)				(x)
#define le32_to_cpu(x)				(x)
#define cpu_to_le32p				(int)
#define le32_to_cpup				(int)
#define __constant_cpu_to_le32(x)	(x)
#define dma_addr_t					unsigned long
#define likely(x)					(x)
#define unlikely(x)					(x)
#define wmb()
#define rmb()


#ifndef _NUCLUES_
#define NU_Create_Semaphore(a,b,c,d)
#define NU_Obtain_Semaphore(x,y)
#define NU_Release_Semaphore(x)
#define NU_Delete_Semaphore(x)
#define NU_Delete_Event_Group(x)
#endif

#endif  /* _USBCONFIG_H_ */


