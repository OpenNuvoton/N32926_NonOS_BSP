/*************************************************************************
 * Nuvoton Electronics Corporation confidential
 *
 * Copyright (c) 2008 by Nuvoton Electronics Corporation
 * All rights reserved
 *
 * FILENAME
 *     usbsupport.h
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
#ifndef  _USBSUPPORT_H_
#define  _USBSUPPORT_H_

/*-------------------------------------------------------------------------
 *   Memory allocation support on Integrator board
 *-------------------------------------------------------------------------*/

enum {
		BOUNDARY_BYTE=1,
		BOUNDARY_HALF_WORD=2,
        BOUNDARY_WORD=4,
        BOUNDARY32=32,
        BOUNDARY64=64,
        BOUNDARY128=128,
        BOUNDARY256=256,
        BOUNDARY512=512,
        BOUNDARY1024=1024,
        BOUNDARY2048=2048,
        BOUNDARY4096=4096
     };

extern UINT32  *_USB_pCurrent;    /* for debug usage */


/*-------------------------------------------------------------------------
 *  Logging System
 *-------------------------------------------------------------------------*/
#ifdef LOG_TO_MEMORY
  #define DRIVER_LOG_MEMORY_SIZE   (64 * 1024)
  extern UINT32  _DriverLogIndex;
  extern UINT8   _DriverLogPool[];
  extern VOID    DriverLogPrintOut(INT);
#else /* !LOG_TO_MEMORY */ 
  #define DriverLogPrintOut(x)
#endif



/*-------------------------------------------------------------------------
 *   Exported Functions
 *-------------------------------------------------------------------------*/
VOID	USB_InitializeMemoryPool(void);
VOID	*USB_malloc(INT wanted_size, INT boundary);
VOID	USB_free(VOID *);
INT32 	USB_available_memory(void);
INT32 	USB_allocated_memory(void);

VOID	USB_WaitMiliseconds(UINT32 msec);
VOID	USB_SetBit(INT nr, UINT8 *map);
VOID	USB_ClearBit(INT nr, UINT8 *map);
INT	 	USB_FindNextZeroBit(UINT8 *map, INT size, INT offset);

VOID	USB_HexDumpBuffer(CHAR *str, UINT8 *buf, INT size);

#endif  /* _USBSUPPORT_H_ */