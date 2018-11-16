/*************************************************************************
 * Nuvoton Electronics Corporation confidential
 *
 * Copyright (c) 2008 by Nuvoton Electronics Corporation
 * All rights reserved
 *
 * FILENAME
 *     usbio.h
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
#ifndef _USBIO_H_
#define _USBIO_H_

#ifndef _USBCONFIG_H_
#include "UsbConfig.h"
#endif

//#define IO_BASE         0xB0000000
//#define NON_CACHE_MASK	0x0
#define NON_CACHE_MASK	0x80000000
#define ECHI_BASE_ADDR	UEHCI20_BA

#define OHCI_BASE_ADDR	UOHCI20_BA


#define readl(addr)          (*(volatile unsigned long *)(addr))
#define writel(x,addr)       ((*(volatile unsigned long *)(addr)) = (volatile unsigned long)x)


/*---  CPU clock speed ---*/
#define HZ               (100)

#define USB_SWAP16(x)    (((x>>8)&0xff)|((x&0xff)<<8))
#define USB_SWAP32(x)    (((x>>24)&0xff)|((x>>8)&0xff00)|((x&0xff00)<<8)|((x&0xff)<<24))



#endif   /* _USBIO_H_ */ 
