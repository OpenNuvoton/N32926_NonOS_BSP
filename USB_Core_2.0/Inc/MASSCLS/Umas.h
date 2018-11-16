/* Driver for USB Mass Storage compliant devices
 * Main Header File
 *
 * $Id: usb.h,v 1.12 2000/12/05 03:33:49 mdharm Exp $
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * Initial work by:
 *   (c) 1999 Michael Gee (michael@linuxspecific.com)
 *
 * This driver is based on the 'USB Mass Storage Class' document. This
 * describes in detail the protocol used to communicate with such
 * devices.  Clearly, the designers had SCSI and ATAPI commands in
 * mind when they created this document.  The commands are all very
 * similar to commands in the SCSI-II and ATAPI specifications.
 *
 * It is important to note that in a number of cases this class
 * exhibits class-specific exemptions from the USB specification.
 * Notably the usage of NAK, STALL and ACK differs from the norm, in
 * that they are used to communicate wait, failed and OK on commands.
 *
 * Also, for certain devices, the interrupt endpoint is used to convey
 * status of a command.
 *
 * Please see http://www.one-eyed-alien.net/~mdharm/linux-usb for more
 * information about this driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _UMAS_H_
#define _UMAS_H_

#ifndef _USB_H_
  #include "usb.h"
#endif  

#include "UmasScsi.h"


#undef DEBUG
#define DEBUG
//#define UMAS_VERBOSE_DEBUG

/* Debug Options */
#ifdef UMAS_VERBOSE_DEBUG
  #define UMAS_DEBUG    sysprintf
  #define UMAS_VDEBUG   sysprintf
#else
  #ifdef DEBUG
     #define UMAS_DEBUG    sysprintf
  #else
     #define UMAS_DEBUG(...)
  #endif
  #define UMAS_VDEBUG(...)
#endif 

#ifdef ECOS
#define sysprintf   diag_printf
#endif

#ifdef ECOS
#define umas_get_ticks()   cyg_current_time()
#else
#define umas_get_ticks()   sysGetTicks(TIMER0)
#endif


/* 
 * GUID definitions
 */

#define GUID(x)            UINT32 x[3]
#define GUID_EQUAL(x, y)   (x[0] == y[0] && x[1] == y[1] && x[2] == y[2])
#define GUID_CLEAR(x)      x[0] = x[1] = x[2] = 0;
#define GUID_NONE(x)       (!x[0] && !x[1] && !x[2])
#define GUID_FORMAT        "%08x%08x%08x"
#define GUID_ARGS(x)       x[0], x[1], x[2]

static __inline VOID  make_guid(UINT32 *pg, UINT16 vendor, UINT16 product, CHAR *serial)
{
    pg[0] = (vendor << 16) | product;
    pg[1] = pg[2] = 0;
    while (*serial) 
    {
       pg[1] <<= 4;
       pg[1] |= pg[2] >> 28;
       pg[2] <<= 4;
       if (*serial >= 'a')
           *serial -= 'a' - 'A';
       pg[2] |= (*serial <= '9' && *serial >= '0') ? *serial - '0' : *serial - 'A' + 10;
       serial++;
    }
}

struct umas_data;

/*
 * Unusual device list definitions 
 */

typedef struct umas_unusual_dev 
{
    const CHAR*  vendorName;
    const CHAR*  productName;
    UINT8   useProtocol;
    UINT8   useTransport;
    INT     (*initFunction)(struct umas_data *);
    UINT32  flags;
} UMAS_UUDEV_T;


/* Flag definitions */
#define UMAS_FL_SINGLE_LUN      0x00000001 	/* allow access to only LUN 0      */
#define UMAS_FL_MODE_XLATE      0x00000002 	/* translate _6 to _10 commands for
                                           		Win/MacOS compatibility */
#define UMAS_FL_START_STOP      0x00000004 	/* ignore START_STOP commands      */
#define UMAS_FL_IGNORE_SER      0x00000010 	/* Ignore the serial number given  */
#define UMAS_FL_SCM_MULT_TARG   0x00000020 	/* supports multiple targets */

#define USB_STOR_STRING_LEN   256

typedef INT  (*trans_cmnd)(SCSI_CMD_T *, struct umas_data *);
typedef INT  (*trans_reset)(struct umas_data *);
typedef VOID (*proto_cmnd)(SCSI_CMD_T*, struct umas_data *);
typedef VOID (*extra_data_destructor)(VOID *);   /* extra data destructor   */


struct umas_drive;

/* we allocate one of these for every device that we remember */
typedef struct umas_data               /* LINUX: struct us_data */
{
    struct umas_data  *next;           /* next device */
    USB_DEV_T     *pusb_dev;           /* this usb_device */
    UINT32        flags;               /* from filter initially */

    /* information about the device -- always good */
    CHAR          vendor[USB_STOR_STRING_LEN];
    CHAR          product[USB_STOR_STRING_LEN];
    CHAR          serial[USB_STOR_STRING_LEN];
    CHAR          *transport_name;
    CHAR          *protocol_name;
    UINT8         subclass;
    UINT8         protocol;
    UINT8         max_lun;

    /* information about the device -- only good if device is attached */
    UINT8         ifnum;               /* interface number   */
    UINT8         ep_in;               /* bulk in endpoint   */
    UINT8         ep_out;              /* bulk out endpoint  */
    USB_EP_DESC_T  *ep_int;            /* interrupt endpoint */ 

    /* function pointers for this device */
    trans_cmnd    transport;           /* transport function     */
    trans_reset   transport_reset;     /* transport device reset */
    proto_cmnd    proto_handler;       /* protocol handler       */

    /* SCSI interfaces */
    GUID(guid);                        /* unique dev id       */
    SCSI_CMD_T    *srb;                /* current srb         */

    INT        	  ip_wanted;           /* is an IRQ expected? (atomic_t) */
    URB_T         *current_urb;        /* non-INT USB requests */
    URB_T         *irq_urb;            /* for USB INT requests */
    UINT8         *irqbuf;             /* buffer for USB IRQ   */
    UINT8         irqdata[2];          /* data from USB IRQ    */

    UMAS_UUDEV_T  *unusual_dev;        /* If unusual device       */
    VOID          *extra;              /* Any extra data          */
    extra_data_destructor   extra_destructor;  /* extra data destructor   */
    INT			  prefer_drive_no;
    struct umas_drive  *drive_list;
} UMAS_DATA_T;


typedef struct umas_drive             
{
    UMAS_DATA_T	  	*umas;
    UINT8			lun_no;
    VOID		  	*client;		   /* file system client data */
    struct umas_drive  *next;          /* next device */
} UMAS_DRIVE_T;



/*======================================================= Global Variables ==*/
/* The list of structures and the protective lock for them */
extern UMAS_DATA_T  *_UmasDeviceList;   /* LINUX: us_list */

/* The structure which defines our driver */
extern USB_DRIVER_T  _UsbMassStorageDriver;   /* LINUX: usb_storage_driver */


/*===================================================== Exported Functions ==*/
extern INT  UMAS_InitUmasDriver(void);
extern VOID UMAS_RemoveUmasDriver(void);

extern INT  UMAS_InitUmasDevice(UMAS_DATA_T *umas);

extern VOID UMAS_ScanAllDevice(void);
extern VOID UMAS_ScanDeviceLun(UMAS_DATA_T *umas);

/*
 * Debug helper routines, in UsbDebug.c 
 */
extern VOID UMAS_DEBUG_ShowCommand(SCSI_CMD_T	*srb);
extern VOID UMAS_DEBUG_PrintScsiCommand(SCSI_CMD_T *cmd);
extern VOID UMAS_DEBUG_ShowSense(UINT8 key, UINT8 asc, UINT8 ascq);

#endif  /* _UMAS_H_ */
