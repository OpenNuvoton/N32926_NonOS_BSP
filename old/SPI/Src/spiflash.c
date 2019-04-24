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

int usiCheckBusy()
{
	volatile UINT32 u32status;
	// check status
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// status command
	outpw(REG_SPI0_TX0, 0x05);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// get status
	while(1)
	{
		outpw(REG_SPI0_TX0, 0xff);
		spiTxLen(0, 0, 8);
		spiActive(0);

		u32status = inpw(REG_SPI0_RX0);		
		if(((u32status & 0xff) & 0x01) != 0x01)
			break;		
	}

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}

INT16 usiReadID()
{
	UINT16 volatile id;

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// command 8 bit
	outpw(REG_SPI0_TX0, 0x90);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// address 24 bit
	outpw(REG_SPI0_TX0, 0x000000);
	spiTxLen(0, 0, 24);
	spiActive(0);

	// data 16 bit
	outpw(REG_SPI0_TX0, 0xffff);
	spiTxLen(0, 0, 16);
	spiActive(0);
	id = inpw(REG_SPI0_RX0) & 0xffff;

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return id;
}


int usiWriteEnable()
{
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	outpw(REG_SPI0_TX0, 0x06);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}

int usiWriteDisable()
{
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	outpw(REG_SPI0_TX0, 0x04);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}

int usiStatusWrite(UINT8 data)
{
	usiWriteEnable();

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// status command
	outpw(REG_SPI0_TX0, 0x01);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// write status
	outpw(REG_SPI0_TX0, data);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}

int usiStatusWrite1(UINT8 data0, UINT8 data1)
{
	usiWriteEnable();

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// status command
	outpw(REG_SPI0_TX0, 0x01);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// write status
	outpw(REG_SPI0_TX0, data0);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_TX0, data1);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	// check status
	usiCheckBusy();

	return Successful;
}

int usiStatusRead(UINT8 cmd, PUINT8 data)
{
	usiWriteEnable();

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// command
	outpw(REG_SPI0_TX0, cmd);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_TX0, 0xff);
	spiTxLen(0, 0, 8);
	spiActive(0);
	*data++ = inpw(REG_SPI0_RX0) & 0xff;	

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}

int usiEONQPIEnable()
{
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	outpw(REG_SPI0_TX0, 0x38);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}



/**************************************************/
INT spiFlashInit(void)
{
	static BOOL bIsSpiFlashOK = 0;
	int volatile loop;
	UINT32 u32APBClk;

	if (!bIsSpiFlashOK)
	{
		outpw(REG_APBCLK, inpw(REG_APBCLK) | SPIMS0_CKE);	// turn on SPI clk
		//Reset SPI controller first
		outpw(REG_APBIPRST, inpw(REG_APBIPRST) | SPI0RST);
		outpw(REG_APBIPRST, inpw(REG_APBIPRST) & ~SPI0RST);
		// delay for time
		// Delay
	    for (loop=0; loop<500; loop++);  
		//configure SPI0 interface, Base Address 0xB800C000

		/* apb clock is 48MHz, output clock is 10MHz */
		u32APBClk = sysGetAPBClock();
		spiIoctl(0, SPI_SET_CLOCK, u32APBClk/1000000, 10000);

		//Startup SPI0 multi-function features, chip select using SS0
		outpw(REG_GPDFUN1, (inpw(REG_GPDFUN1)  & ~0xFFFF0000) | 0x22220000);
#ifdef __OPT_O2DN__		
		outpw(REG_GPEFUN0, (inpw(REG_GPEFUN0)  & ~(MF_GPE0 |MF_GPE1)) | 0x33);
#else
		outpw(REG_GPEFUN1, (inpw(REG_GPEFUN1)  & ~(MF_GPE8 |MF_GPE9)) | 0x44);
#endif

		outpw(REG_SPI0_SSR, 0x00);		// CS active low
		outpw(REG_SPI0_CNTRL, 0x04);		// Tx: falling edge, Rx: rising edge

		if ((loop=usiReadID()) == -1)
		{
			sysprintf("read id error !! [0x%x]\n", loop&0xffff);
			return -1;
		}

		sysprintf("SPI flash id [0x%x]\n", loop&0xffff);
		usiStatusWrite(0x00);	// clear block protect

		// check status
		usiCheckBusy();		

		bIsSpiFlashOK = 1;
	}	
	return 0;
 }

INT spiFlashReset(void)
{
	// check status
	usiCheckBusy();

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// enable reset command
	outpw(REG_SPI0_TX0, 0x66);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// reset command
	outpw(REG_SPI0_TX0, 0x99);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	sysDelay(5);

	usiStatusWrite(0x00);	// clear block protect

	return Successful;
}


INT spiFlashEraseSector(UINT32 addr, UINT32 secCount)
{
	int volatile i;

	if ((addr % (4*1024)) != 0)
		return -1;

	for (i=0; i<secCount; i++)
	{
		usiWriteEnable();

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// erase command
		outpw(REG_SPI0_TX0, 0x20);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0, addr+i*4*1024);
			spiTxLen(0, 0, 32);
			spiActive(0);
		}
		else
		{
			outpw(REG_SPI0_TX0, addr+i*4*1024);
			spiTxLen(0, 0, 24);
			spiActive(0);
		}

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

		// check status
		usiCheckBusy();
	}

	return Successful;
}


INT spiFlashEraseBlock(UINT32 addr, UINT32 blockCount)
{
	int volatile i;

	if ((addr % (64*1024)) != 0)
		return -1;

	for (i=0; i<blockCount; i++)
	{
		usiWriteEnable();

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// erase command
		outpw(REG_SPI0_TX0, 0xd8);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0, addr+i*64*1024);
			spiTxLen(0, 0, 32);
			spiActive(0);
		}
		else
		{
			outpw(REG_SPI0_TX0, addr+i*64*1024);
			spiTxLen(0, 0, 24);
			spiActive(0);
		}

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

		// check status
		usiCheckBusy();
	}

	return Successful;
}


INT spiFlashEraseAll(void)
{
	usiWriteEnable();

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	outpw(REG_SPI0_TX0, 0xc7);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	// check status
	usiCheckBusy();

	return Successful;
}


INT spiFlashWrite(UINT32 addr, UINT32 len, UINT8 *buf)
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

		usiWriteEnable();

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// write command
		outpw(REG_SPI0_TX0, 0x02);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0, addr+i*256);
			spiTxLen(0, 0, 32);
			spiActive(0);
		}
		else
		{
			outpw(REG_SPI0_TX0, addr+i*256);
			spiTxLen(0, 0, 24);
			spiActive(0);
		}

		spiSetByteEndin(0, eDRVSPI_ENABLE);

		// write data
		while (page > 0)
		{			
			spiTxLen(0, 0, 32);
			outpw(REG_SPI0_TX0, *p32tmp);			
			spiActive(0);
			p32tmp++;			
			page -=4;

			if(page < 4)
			{
				if(page > 0)
				{
					spiSetByteEndin(0, eDRVSPI_DISABLE);
					ptr = (PUINT8)p32tmp;	
					
					for (j=0; j<(page %4); j++)
					{
						spiTxLen(0, 0, 8);
						outpw(REG_SPI0_TX0, *ptr);						
						spiActive(0);
						ptr++;			
						page -=1;
					}
				}
			}
			
		}

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

		spiSetByteEndin(0, eDRVSPI_DISABLE);

		// check status
		usiCheckBusy();
	}

	return Successful;
}


INT spiFlashRead(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;	
	PUINT8 ptr;
	PUINT32 p32tmp;	

	p32tmp = (PUINT32)buf;	

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// read command
	outpw(REG_SPI0_TX0, 03);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// address
	if(g_u8Is4ByteMode == TRUE)
	{
		outpw(REG_SPI0_TX0, addr);
		spiTxLen(0, 0, 32);
		spiActive(0);
	}
	else
	{
		outpw(REG_SPI0_TX0, addr);
		spiTxLen(0, 0, 24);
		spiActive(0);
	}

	spiSetByteEndin(0, eDRVSPI_ENABLE);

	// data	 
	for (i=0; i<len/4; i++)
	{
		spiTxLen(0, 0, 32);
		outpw(REG_SPI0_TX0, 0xffffffff);		
		spiActive(0);
		*p32tmp = inp32(REG_SPI0_RX0);
		p32tmp ++;		
	}	

	spiSetByteEndin(0, eDRVSPI_DISABLE);
	
	if(len % 4)
	{
		ptr = (PUINT8)p32tmp;		
		
		for (i=0; i<(len %4); i++)
		{
			spiTxLen(0, 0, 8);
			outpb(REG_SPI0_TX0, 0xff);			
			spiActive(0);
			*ptr++ = inpb(REG_SPI0_RX0);
		}		
	}	

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	return Successful;
}

INT spiFlashQuadWrite(UINT32 addr, UINT32 len, UINT8 *buf)
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

		usiWriteEnable();

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// write command
		outpw(REG_SPI0_TX0, 0x32);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0, addr+i*256);
			spiTxLen(0, 0, 32);
			spiActive(0);
		}
		else
		{
			outpw(REG_SPI0_TX0, addr+i*256);
			spiTxLen(0, 0, 24);
			spiActive(0);
		}

		outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);
			
		spiSetByteEndin(0, eDRVSPI_ENABLE);

		// write data
		while (page > 0)
		{	
			spiTxLen(0, 0, 32);
			outpw(REG_SPI0_TX0, *p32tmp);			
			spiActive(0);
			p32tmp++;			
			page -=4;

			if(page < 4)
			{
				if(page > 0)
				{
					spiSetByteEndin(0, eDRVSPI_DISABLE);
					ptr = (PUINT8)p32tmp;	
					
					for (j=0; j<(page %4); j++)
					{
						spiTxLen(0, 0, 8);
						outpw(REG_SPI0_TX0, *ptr);						
						spiActive(0);
						ptr++;			
						page -=1;
					}
				}
			}
			
		}
		
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0
		
		spiSetByteEndin(0, eDRVSPI_DISABLE);

		outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);

		// check status
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// status command
		outpw(REG_SPI0_TX0, 0x05);
		spiTxLen(0, 0, 8);
		spiActive(0);
		
		// get status
		while(1)
		{
			outpw(REG_SPI0_TX0, 0xff);
			spiTxLen(0, 0, 8);
			spiActive(0);
			if (((inpw(REG_SPI0_RX0) & 0xff) & 0x01) != 0x01)
				break;
		}
		
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0	
		
	}	
	
	return Successful;
}

INT spiFlashFastReadQuad(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;
	PUINT8 ptr;
	PUINT32 p32tmp;	

	p32tmp = (PUINT32)buf;		

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// read command
	outpw(REG_SPI0_TX0, 0x6B);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// address
	if(g_u8Is4ByteMode == TRUE)
	{
		outpw(REG_SPI0_TX0, addr);
		spiTxLen(0, 0, 32);
		spiActive(0);
	}
	else
	{
		outpw(REG_SPI0_TX0, addr);
		spiTxLen(0, 0, 24);
		spiActive(0);
	}

	// dummy clock
	outpw(REG_SPI0_TX0, 0xFF);
	spiTxLen(0, 0, 8);
	spiActive(0);	
	
	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);

	spiSetByteEndin(0, eDRVSPI_ENABLE);	

	// data
	for (i=0; i<len/4; i++)
	{	
		spiTxLen(0, 0, 32);
		outpw(REG_SPI0_TX0, 0xffffffff);		
		spiActive(0);
		*p32tmp = inpw(REG_SPI0_RX0);
		p32tmp ++;
	}

	spiSetByteEndin(0, eDRVSPI_DISABLE);
	
	if(len % 4)
	{
		ptr = (PUINT8)p32tmp;		
		
		for (i=0; i<(len %4); i++)
		{
			spiTxLen(0, 0, 8);
			outpb(REG_SPI0_TX0, 0xff);			
			spiActive(0);
			*ptr++ = inpb(REG_SPI0_RX0);
		}		
	}
	
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0
	
	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);

	return Successful;
}

INT spiEONFlashQuadWrite(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile count=0, page, i, j;	
	PUINT8 ptr;
	PUINT32 p32tmp;

	p32tmp = (PUINT32)buf;

	usiEONQPIEnable();	
		
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

		outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);		

		usiWriteEnable();

		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// write command
		outpw(REG_SPI0_TX0, 0x02);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		if(g_u8Is4ByteMode == TRUE)
		{
			outpw(REG_SPI0_TX0, addr+i*256);
			spiTxLen(0, 0, 32);
			spiActive(0);
		}
		else
		{
			outpw(REG_SPI0_TX0, addr+i*256);
			spiTxLen(0, 0, 24);
			spiActive(0);		
		}
			
		spiSetByteEndin(0, eDRVSPI_ENABLE);

		// write data
		while (page > 0)
		{	
			spiTxLen(0, 0, 32);
			outpw(REG_SPI0_TX0, *p32tmp);			
			spiActive(0);
			p32tmp++;			
			page -=4;

			if(page < 4)
			{
				if(page > 0)
				{
					spiSetByteEndin(0, eDRVSPI_DISABLE);
					ptr = (PUINT8)p32tmp;	
					
					for (j=0; j<(page %4); j++)
					{
						spiTxLen(0, 0, 8);
						outpw(REG_SPI0_TX0, *ptr);						
						spiActive(0);
						ptr++;			
						page -=1;
					}
				}
			}
			
		}
		
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0
		
		spiSetByteEndin(0, eDRVSPI_DISABLE);
		
		// check status
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

		// status command
		outpw(REG_SPI0_TX0, 0x05);
		spiTxLen(0, 0, 8);
		spiActive(0);

		outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);
		
		// get status
		while(1)
		{
			outpw(REG_SPI0_TX0, 0xff);
			spiTxLen(0, 0, 8);
			spiActive(0);
			if (((inpw(REG_SPI0_RX0) & 0xff) & 0x01) != 0x01)
				break;
		}
		
		outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0	
		
	}	
	
	return Successful;
}

INT spiEONFlashFastReadQuad(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;
	PUINT8 ptr;
	PUINT32 p32tmp;

	p32tmp = (PUINT32)buf;

	usiEONQPIEnable();

	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// read command
	outpw(REG_SPI0_TX0, 0xEB);
	spiTxLen(0, 0, 8);
	spiActive(0);	

	// address
	if(g_u8Is4ByteMode == TRUE)
	{
		outpw(REG_SPI0_TX0, addr);
		spiTxLen(0, 0, 32);
		spiActive(0);
	}
	else
	{
		outpw(REG_SPI0_TX0, addr);
		spiTxLen(0, 0, 24);
		spiActive(0);
	}

	// dummy clock
	for(i=0; i<3; i++)
	{
		outpw(REG_SPI0_TX0, 0xFF);
		spiTxLen(0, 0, 8);
		spiActive(0);	
	}	

	spiSetByteEndin(0, eDRVSPI_ENABLE);	

	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);

	// data
	for (i=0; i<len/4; i++)
	{	
		spiTxLen(0, 0, 32);
		outpw(REG_SPI0_TX0, 0xffffffff);		
		spiActive(0);
		*p32tmp = inpw(REG_SPI0_RX0);
		p32tmp ++;
	}

	spiSetByteEndin(0, eDRVSPI_DISABLE);
	
	if(len % 4)
	{
		ptr = (PUINT8)p32tmp;		
		
		for (i=0; i<(len %4); i++)
		{
			spiTxLen(0, 0, 8);
			outpb(REG_SPI0_TX0, 0xff);			
			spiActive(0);
			*ptr++ = inpb(REG_SPI0_RX0);
		}		
	}
	
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0
	
	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);

	return Successful;
}

INT spiFlashEnter4ByteMode(void)
{
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// enter 4 byte mode command
	outpw(REG_SPI0_TX0, 0xB7);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	g_u8Is4ByteMode = TRUE;

	return Successful;
}

INT spiFlashExit4ByteMode(void)
{
	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);	// CS0

	// exit 4 byte mode command
	outpw(REG_SPI0_TX0, 0xE9);
	spiTxLen(0, 0, 8);
	spiActive(0);

	outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);	// CS0

	g_u8Is4ByteMode = FALSE;

	return Successful;
}


