/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
/****************************************************************************
 * 
 * FILENAME
 *     spiflash.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This file contains SPI flash library APIs.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
 *     10/12/07      Create Ver 1.0
 *     06/12/09      Add SPI flash
 *
 * REMARK
 *     None
 **************************************************************************/
/* Header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wblib.h"

#include "w55fa92_reg.h"
#include "w55fa92_spi.h"

UINT8 g_u8Is4ByteMode;
//#define SPI_CLOCK_MODE	0
#define SPI_CLOCK_MODE	3

int usiCheckBusy(UINT32 spiPort, UINT32 SSPin)
{
	volatile UINT32 u32status;
	// check status
	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// status command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x05);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	// get status
	while(1)
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		u32status = inpw(REG_SPI0_RX0 + 0x400 * spiPort);		
		if(((u32status & 0xff) & 0x01) != 0x01)
			break;		
	}

	spiSSDisable(spiPort, SSPin);

	return Successful;
}

INT16 usiReadID(UINT32 spiPort, UINT32 SSPin)
{
	UINT16 volatile id;

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// command 8 bit
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x90);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	// address 24 bit
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x000000);
	spiTxLen(spiPort, 0, 24);
	spiActive(spiPort);

	// data 16 bit
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xffff);
	spiTxLen(spiPort, 0, 16);
	spiActive(spiPort);
	id = inpw(REG_SPI0_RX0 + 0x400 * spiPort) & 0xffff;

	spiSSDisable(spiPort, SSPin);

	return id;
}


int usiWriteEnable(UINT32 spiPort, UINT32 SSPin)
{
	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x06);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	return Successful;
}

int usiWriteDisable(UINT32 spiPort, UINT32 SSPin)
{
	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x04);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	return Successful;
}

int usiStatusWrite(UINT32 spiPort, UINT32 SSPin, UINT8 data)
{
	usiWriteEnable(spiPort, SSPin);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// status command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x01);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	// write status
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, data);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	return Successful;
}

int usiStatusWrite1(UINT32 spiPort, UINT32 SSPin, UINT8 data0, UINT8 data1)
{
	usiWriteEnable(spiPort, SSPin);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// status command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x01);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	// write status
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, data0);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	outpw(REG_SPI0_TX0 + 0x400 * spiPort, data1);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	// check status
	usiCheckBusy(spiPort, SSPin);

	return Successful;
}

int usiStatusRead(UINT32 spiPort, UINT32 SSPin, UINT8 cmd, PUINT8 data)
{
	usiWriteEnable(spiPort, SSPin);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, cmd);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);
	*data++ = inpw(REG_SPI0_RX0 + 0x400 * spiPort) & 0xff;	

	spiSSDisable(spiPort, SSPin);

	return Successful;
}

int usiEONQPIEnable(UINT32 spiPort, UINT32 SSPin)
{
	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x38);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	return Successful;
}



/**************************************************/
INT spiFlashInit(UINT32 spiPort, UINT32 SSPin)
{
	static BOOL bIsSpiFlashOK = 0;
	int volatile loop;
	UINT32 u32APBClk;

	if (!bIsSpiFlashOK)
	{
		outpw(REG_APBCLK, inpw(REG_APBCLK) | SPIMS0_CKE);	// turn on SPI clk
		//Reset SPI controller first
//		outpw(REG_APBIPRST, inpw(REG_APBIPRST) | SPI0RST);
//		outpw(REG_APBIPRST, inpw(REG_APBIPRST) & ~SPI0RST);
		// delay for time
		// Delay
	    for (loop=0; loop<500; loop++);  
		//configure SPI0 interface, Base Address 0xB800C000

		/* apb clock is 48MHz, output clock is 10MHz */
		u32APBClk = sysGetAPBClock();
		spiIoctl(spiPort, SPI_SET_CLOCK, u32APBClk/1000000, 15000);

		//Startup SPI0 multi-function features, chip select using SS0
		outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1)  & ~0xFFFF0000) | 0x22220000);
#ifdef __OPT_O2DN__		
		outpw(REG_GPEFUN0, (inpw(REG_GPEFUN0)  & ~(MF_GPE0 |MF_GPE1)) | 0x33);
#else
		outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1)  & ~(MF_GPE8 |MF_GPE9)) | 0x44);
#endif

		outpw(REG_SPI0_SSR + 0x400 * spiPort, 0x00);		// CS active low
		outpw(REG_SPI0_CNTRL + 0x400 * spiPort, 0x04);		// Tx: falling edge, Rx: rising edge

		if ((loop=usiReadID(spiPort, SSPin)) == -1)
		{
			sysprintf("read id error !! [0x%x]\n", loop&0xffff);
			return -1;
		}

		sysprintf("SPI flash id [0x%x]\n", loop&0xffff);
		usiStatusWrite(spiPort, SSPin, 0x00);	// clear block protect

		// check status
		usiCheckBusy(spiPort, SSPin);		

		bIsSpiFlashOK = 1;
	}	
	return 0;
 }

INT spiFlashReset(UINT32 spiPort, UINT32 SSPin)
{
	// check status
	usiCheckBusy(spiPort, SSPin);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// enable reset command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x66);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// reset command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x99);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	sysDelay(5);

	usiStatusWrite(spiPort, SSPin, 0x00);	// clear block protect

	return Successful;
}


INT spiFlashEraseSector(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 secCount)
{
	int volatile i;

	if ((addr % (4*1024)) != 0)
		return -1;

	for (i=0; i<secCount; i++)
	{
		usiWriteEnable(spiPort, SSPin);

		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// erase command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x20);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*4*1024);
			spiTxLen(spiPort, 0, 32);
			spiActive(spiPort);
		}
		else
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*4*1024);
			spiTxLen(spiPort, 0, 24);
			spiActive(spiPort);
		}

		spiSSDisable(spiPort, SSPin);

		// check status
		usiCheckBusy(spiPort, SSPin);
	}

	return Successful;
}


INT spiFlashEraseBlock(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 blockCount)
{
	int volatile i;

	if ((addr % (64*1024)) != 0)
		return -1;

	for (i=0; i<blockCount; i++)
	{
		usiWriteEnable(spiPort, SSPin);

		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// erase command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xd8);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*64*1024);
			spiTxLen(spiPort, 0, 32);
			spiActive(spiPort);
		}
		else
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*64*1024);
			spiTxLen(spiPort, 0, 24);
			spiActive(spiPort);
		}

		spiSSDisable(spiPort, SSPin);

		// check status
		usiCheckBusy(spiPort, SSPin);
	}

	return Successful;
}


INT spiFlashEraseAll(UINT32 spiPort, UINT32 SSPin)
{
	usiWriteEnable(spiPort, SSPin);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xc7);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	// check status
	usiCheckBusy(spiPort, SSPin);

	return Successful;
}


INT spiFlashWrite(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile count=0, page, i, j;	
	PUINT8 ptr;
	PUINT32 p32tmp;

	p32tmp = (PUINT32)buf;	

	count = len / 256;
	if ((len % 256) != 0)
		count++;

	for (i=0; i<count; i++)
	{
		// check data len
		if (len >= 256)
		{
			page = 256;
			len = len - 256;
		}
		else
			page = len;

		usiWriteEnable(spiPort, SSPin);

		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// write command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x02);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*256);
			spiTxLen(spiPort, 0, 32);
			spiActive(spiPort);
		}
		else
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*256);
			spiTxLen(spiPort, 0, 24);
			spiActive(spiPort);
		}

		spiSetByteEndin(spiPort, eDRVSPI_ENABLE);

		// write data
		while (page > 0)
		{			
			spiTxLen(spiPort, 0, 32);
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, *p32tmp);			
			spiActive(spiPort);
			p32tmp++;			
			page -=4;

			if(page < 4)
			{
				if(page > 0)
				{
					spiSetByteEndin(spiPort, eDRVSPI_DISABLE);
					ptr = (PUINT8)p32tmp;	
					
					for (j=0; j<(page %4); j++)
					{
						spiTxLen(spiPort, 0, 8);
						outpw(REG_SPI0_TX0 + 0x400 * spiPort, *ptr);						
						spiActive(spiPort);
						ptr++;			
						page -=1;
					}
				}
			}
			
		}

		spiSetByteEndin(spiPort, eDRVSPI_DISABLE);		

		spiSSDisable(spiPort, SSPin);

		// check status
		usiCheckBusy(spiPort, SSPin);
	}

	return Successful;
}


INT spiFlashRead(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;	
	PUINT8 ptr;
	PUINT32 p32tmp;	

	p32tmp = (PUINT32)buf;	

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// read command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 03);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	// address
	if(g_u8Is4ByteMode == TRUE)
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr);
		spiTxLen(spiPort, 0, 32);
		spiActive(spiPort);
	}
	else
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr);
		spiTxLen(spiPort, 0, 24);
		spiActive(spiPort);
	}

	spiSetByteEndin(spiPort, eDRVSPI_ENABLE);

	// data	 
	for (i=0; i<len/4; i++)
	{
		spiTxLen(spiPort, 0, 32);
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xffffffff);		
		spiActive(spiPort);
		*p32tmp = inp32(REG_SPI0_RX0 + 0x400 * spiPort);
		p32tmp ++;		
	}	

	spiSetByteEndin(spiPort, eDRVSPI_DISABLE);
	
	if(len % 4)
	{
		ptr = (PUINT8)p32tmp;		
		
		for (i=0; i<(len %4); i++)
		{
			spiTxLen(spiPort, 0, 8);
			outpb(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);			
			spiActive(spiPort);
			*ptr++ = inpb(REG_SPI0_RX0 + 0x400 * spiPort);
		}		
	}	

	spiSSDisable(spiPort, SSPin);

	return Successful;
}

INT spiFlashQuadWrite(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile count=0, page, i, j;		
	PUINT8 ptr;
	PUINT32 p32tmp;

	p32tmp = (PUINT32)buf;		
		
	count = len / 256;
	if ((len % 256) != 0)
		count++;

	for (i=0; i<count; i++)
	{		
		// check data len
		if (len >= 256)
		{
			page = 256;
			len = len - 256;
		}
		else
			page = len;

		usiWriteEnable(spiPort, SSPin);

		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// write command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x32);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*256);
			spiTxLen(spiPort, 0, 32);
			spiActive(spiPort);
		}
		else
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*256);
			spiTxLen(spiPort, 0, 24);
			spiActive(spiPort);
		}

		outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);
			
		spiSetByteEndin(spiPort, eDRVSPI_ENABLE);

		// write data
		while (page > 0)
		{	
			spiTxLen(spiPort, 0, 32);
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, *p32tmp);			
			spiActive(spiPort);
			p32tmp++;			
			page -=4;

			if(page < 4)
			{
				if(page > 0)
				{
					spiSetByteEndin(spiPort, eDRVSPI_DISABLE);
					ptr = (PUINT8)p32tmp;	
					
					for (j=0; j<(page %4); j++)
					{
						spiTxLen(spiPort, 0, 8);
						outpw(REG_SPI0_TX0 + 0x400 * spiPort, *ptr);						
						spiActive(spiPort);
						ptr++;			
						page -=1;
					}
				}
			}
			
		}
				
		spiSetByteEndin(spiPort, eDRVSPI_DISABLE);

		outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(BIT_MODE)) |eDRVSPI_1BITS);

		spiSSDisable(spiPort, SSPin);		

		// check status
		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// status command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x05);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);
		
		// get status
		while(1)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);
			spiTxLen(spiPort, 0, 8);
			spiActive(spiPort);
			if (((inpw(REG_SPI0_RX0 + 0x400 * spiPort) & 0xff) & 0x01) != 0x01)
				break;
		}
		
		spiSSDisable(spiPort, SSPin);	
		
	}	
	
	return Successful;
}

INT spiFlashFastReadQuad(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;
	PUINT8 ptr;
	PUINT32 p32tmp;	

	p32tmp = (PUINT32)buf;		

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// read command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x6B);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	// address
	if(g_u8Is4ByteMode == TRUE)
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr);
		spiTxLen(spiPort, 0, 32);
		spiActive(spiPort);
	}
	else
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr);
		spiTxLen(spiPort, 0, 24);
		spiActive(spiPort);
	}

	// dummy clock
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xFF);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);	
	
	outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);

	spiSetByteEndin(spiPort, eDRVSPI_ENABLE);	

	// data
	for (i=0; i<len/4; i++)
	{	
		spiTxLen(spiPort, 0, 32);
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xffffffff);		
		spiActive(spiPort);
		*p32tmp = inpw(REG_SPI0_RX0 + 0x400 * spiPort);
		p32tmp ++;
	}

	spiSetByteEndin(spiPort, eDRVSPI_DISABLE);
	
	if(len % 4)
	{
		ptr = (PUINT8)p32tmp;		
		
		for (i=0; i<(len %4); i++)
		{
			spiTxLen(spiPort, 0, 8);
			outpb(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);			
			spiActive(spiPort);
			*ptr++ = inpb(REG_SPI0_RX0 + 0x400 * spiPort);
		}		
	}
		
	outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(BIT_MODE)) |eDRVSPI_1BITS);

	spiSSDisable(spiPort, SSPin);	

	return Successful;
}

INT spiEONFlashQuadWrite(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile count=0, page, i, j;	
	PUINT8 ptr;
	PUINT32 p32tmp;

	p32tmp = (PUINT32)buf;

	usiEONQPIEnable(spiPort, SSPin);	
		
	count = len / 256;
	if ((len % 256) != 0)
		count++;

	for (i=0; i<count; i++)
	{		
		// check data len
		if (len >= 256)
		{
			page = 256;
			len = len - 256;
		}
		else
			page = len;

		outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);		

		usiWriteEnable(spiPort, SSPin);

		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// write command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x02);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*256);
			spiTxLen(spiPort, 0, 32);
			spiActive(spiPort);
		}
		else
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr+i*256);
			spiTxLen(spiPort, 0, 24);
			spiActive(spiPort);		
		}
			
		spiSetByteEndin(spiPort, eDRVSPI_ENABLE);

		// write data
		while (page > 0)
		{	
			spiTxLen(spiPort, 0, 32);
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, *p32tmp);			
			spiActive(spiPort);
			p32tmp++;			
			page -=4;

			if(page < 4)
			{
				if(page > 0)
				{
					spiSetByteEndin(spiPort, eDRVSPI_DISABLE);
					ptr = (PUINT8)p32tmp;	
					
					for (j=0; j<(page %4); j++)
					{
						spiTxLen(spiPort, 0, 8);
						outpw(REG_SPI0_TX0 + 0x400 * spiPort, *ptr);						
						spiActive(spiPort);
						ptr++;			
						page -=1;
					}
				}
			}
			
		}
				
		spiSetByteEndin(spiPort, eDRVSPI_DISABLE);

		spiSSDisable(spiPort, SSPin);		
		
		// check status
		spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

		// status command
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0x05);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);

		outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);
		
		// get status
		while(1)
		{
			outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);
			spiTxLen(spiPort, 0, 8);
			spiActive(spiPort);
			if (((inpw(REG_SPI0_RX0 + 0x400 * spiPort) & 0xff) & 0x01) != 0x01)
				break;
		}
		
		spiSSDisable(spiPort, SSPin);	
		
	}	
	
	return Successful;
}

INT spiEONFlashFastReadQuad(UINT32 spiPort, UINT32 SSPin, UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;
	PUINT8 ptr;
	PUINT32 p32tmp;

	p32tmp = (PUINT32)buf;

	usiEONQPIEnable(spiPort, SSPin);

	outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);

	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// read command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xEB);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);	

	// address
	if(g_u8Is4ByteMode == TRUE)
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr);
		spiTxLen(spiPort, 0, 32);
		spiActive(spiPort);
	}
	else
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, addr);
		spiTxLen(spiPort, 0, 24);
		spiActive(spiPort);
	}

	// dummy clock
	for(i=0; i<3; i++)
	{
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xFF);
		spiTxLen(spiPort, 0, 8);
		spiActive(spiPort);	
	}	

	spiSetByteEndin(spiPort, eDRVSPI_ENABLE);	

	outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);

	// data
	for (i=0; i<len/4; i++)
	{	
		spiTxLen(spiPort, 0, 32);
		outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xffffffff);		
		spiActive(spiPort);
		*p32tmp = inpw(REG_SPI0_RX0 + 0x400 * spiPort);
		p32tmp ++;
	}

	spiSetByteEndin(spiPort, eDRVSPI_DISABLE);
	
	if(len % 4)
	{
		ptr = (PUINT8)p32tmp;		
		
		for (i=0; i<(len %4); i++)
		{
			spiTxLen(spiPort, 0, 8);
			outpb(REG_SPI0_TX0 + 0x400 * spiPort, 0xff);			
			spiActive(spiPort);
			*ptr++ = inpb(REG_SPI0_RX0 + 0x400 * spiPort);
		}		
	}
		
	outpw(REG_SPI0_CNTRL + 0x400 * spiPort, (inpw(REG_SPI0_CNTRL + 0x400 * spiPort) & ~(BIT_MODE)) |eDRVSPI_1BITS);

	spiSSDisable(spiPort, SSPin);	

	return Successful;
}

INT spiFlashEnter4ByteMode(UINT32 spiPort, UINT32 SSPin)
{
	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// enter 4 byte mode command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xB7);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	g_u8Is4ByteMode = TRUE;

	return Successful;
}

INT spiFlashExit4ByteMode(UINT32 spiPort, UINT32 SSPin)
{
	spiSSEnable(spiPort, SSPin, SPI_CLOCK_MODE);

	// exit 4 byte mode command
	outpw(REG_SPI0_TX0 + 0x400 * spiPort, 0xE9);
	spiTxLen(spiPort, 0, 8);
	spiActive(spiPort);

	spiSSDisable(spiPort, SSPin);

	g_u8Is4ByteMode = FALSE;

	return Successful;
}


