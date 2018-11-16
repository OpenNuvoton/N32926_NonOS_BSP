#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wblib.h"
#include "w55fa92_edma.h"
#include "w55fa92_spi.h"

#define TEST_SIZE	100 * 1024
__align(4096) UINT8 WriteBuffer[TEST_SIZE];
__align(4096) UINT8 ReadBuffer[TEST_SIZE];

static INT32 g_PdmaCh = 0;
volatile static BOOL g_bPdmaInt = FALSE;

extern int usiWriteEnable(void);
extern int usiCheckBusy(void);
extern int usiStatusWrite1(UINT8 data0, UINT8 data1);
extern int usiStatusRead(UINT8 cmd, PUINT8 data);

void PdmaCallback_SPI(unsigned int arg)
{ 		 	
	g_bPdmaInt = TRUE;
}

int initSPIPDMA_Write(UINT32 src_addr, UINT32 dma_length)
{
	
	g_PdmaCh = PDMA_FindandRequest();
	EDMA_SetAPB(g_PdmaCh,			//int channel, 
						eDRVEDMA_SPIMS0,			//E_DRVEDMA_APB_DEVICE eDevice, 
						eDRVEDMA_WRITE_APB,		//E_DRVEDMA_APB_RW eRWAPB, 
						eDRVEDMA_WIDTH_32BITS);	//E_DRVEDMA_TRANSFER_WIDTH eTransferWidth	

	EDMA_SetupHandlers(g_PdmaCh, 		//int channel
						eDRVEDMA_BLKD, 			//int interrupt,	
						PdmaCallback_SPI, 				//void (*irq_handler) (void *),
						NULL);					//void *data

	EDMA_SetWrapINTType(g_PdmaCh , NULL);								

	EDMA_SetDirection(g_PdmaCh , eDRVEDMA_DIRECTION_INCREMENTED, eDRVEDMA_DIRECTION_FIXED);


	EDMA_SetupSingle(g_PdmaCh,		// int channel, 
								src_addr,		// unsigned int src_addr,  (ADC data port physical address) 
								REG_SPI0_TX0, //phaddrrecord,		// unsigned int dest_addr,
								dma_length);	// unsigned int dma_length /* Lenth equal 2 half buffer */

	return Successful;
}

int initSPIPDMA_Read(UINT32 dest_addr, UINT32 dma_length)
{
	
	g_PdmaCh = PDMA_FindandRequest();
	EDMA_SetAPB(g_PdmaCh,			//int channel, 
						eDRVEDMA_SPIMS0,			//E_DRVEDMA_APB_DEVICE eDevice, 
						eDRVEDMA_READ_APB,		//E_DRVEDMA_APB_RW eRWAPB, 
						eDRVEDMA_WIDTH_32BITS);	//E_DRVEDMA_TRANSFER_WIDTH eTransferWidth	

	EDMA_SetupHandlers(g_PdmaCh, 		//int channel
						eDRVEDMA_BLKD, 			//int interrupt,	
						PdmaCallback_SPI, 				//void (*irq_handler) (void *),
						NULL);					//void *data

	EDMA_SetWrapINTType(g_PdmaCh , NULL);								

	EDMA_SetDirection(g_PdmaCh , eDRVEDMA_DIRECTION_FIXED, eDRVEDMA_DIRECTION_INCREMENTED);


	EDMA_SetupSingle(g_PdmaCh,		// int channel, 
								REG_SPI0_RX0,		// unsigned int src_addr,  (ADC data port physical address) 
								dest_addr, //phaddrrecord,		// unsigned int dest_addr,
								dma_length);	// unsigned int dma_length /* Lenth equal 2 half buffer */

	return Successful;
}


INT spiFlashPDMAWrite(UINT32 addr, UINT32 len, PUINT8 buf)
{
	int count=0, page, i, j;	
	UINT8 data;	
	PUINT8 ptr;

	usiStatusWrite1(0x00, 0x00);	// clear block protect , disable QE

	usiStatusRead(0x35, &data);
	sysprintf("SpiFlash Register-2 Status 0x%x \n", data);
	
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

		//sysprintf("len %d page %d\n",len , page);
		usiWriteEnable();

		spiEnable(0);	// CS0

		// write command
		outpw(REG_SPI0_TX0, 0x02);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		outpw(REG_SPI0_TX0, addr+i*256);
		spiTxLen(0, 0, 24);
		spiActive(0);	

		// write data
		if(page == 256)
		{
			spiSetByteEndin(0, eDRVSPI_ENABLE);
			spiTxLen(0, 0, 32);		

			initSPIPDMA_Write((UINT32)buf, page);
			buf = buf + page;

			EDMA_Trigger(g_PdmaCh);
			outp32(REG_SPI0_EDMA, (inp32(REG_SPI0_EDMA) & ~0x03) | EDMA_GO);							

			while(g_bPdmaInt == FALSE);	
			g_bPdmaInt = FALSE;

			while(spiIsBusy(0));

			EDMA_Free(g_PdmaCh); 

			outp32(REG_SPI0_EDMA, inp32(REG_SPI0_EDMA) & ~EDMA_GO);
		}
		else
		{
			spiSetByteEndin(0, eDRVSPI_DISABLE);
			spiTxLen(0, 0, 8);
			
			ptr = buf;
			for (j=0; j<page; j++)
			{						
				outpw(REG_SPI0_TX0, *ptr);						
				spiActive(0);
				ptr++;						
			}			
		}			

		spiDisable(0);	// CS0

		spiSetByteEndin(0, eDRVSPI_DISABLE);
		spiTxLen(0, 0, 8);
			
		// check status
		usiCheckBusy();
	}

	return Successful;
}

INT spiFlashPDMARead(UINT32 addr, UINT32 len, PUINT8 buf)
{
	int volatile i;
	PUINT8 ptr;	
	UINT8 data;
	UINT32 u32len;
		
	usiStatusWrite1(0x00, 0x00);	// clear block protect , disable QE

	usiStatusRead(0x35, &data);
	sysprintf("SpiFlash Register-2 Status 0x%x \n", data);
	
	spiEnable(0);	// CS0

	// read command
	outpw(REG_SPI0_TX0, 0x0b);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// address
	outpw(REG_SPI0_TX0, addr);
	spiTxLen(0, 0, 24);
	spiActive(0);

	// dummy byte
	outp32(REG_SPI0_TX0, 0xff);
	spiTxLen(0, 0, 8);
	spiActive(0);

	spiSetByteEndin(0, eDRVSPI_ENABLE);
	spiTxLen(0, 0, 32);

	u32len = len;

	if(len % 4)
		u32len = (len/4)*4;

	initSPIPDMA_Read((UINT32)buf, u32len);
	
	EDMA_Trigger(g_PdmaCh);
	outp32(REG_SPI0_EDMA, (inp32(REG_SPI0_EDMA) & ~0x03) | (EDMA_RW | EDMA_GO));	
	outp32(REG_SPI0_CNTRL, inp32(REG_SPI0_CNTRL) |GO_BUSY);	    
	
	while(g_bPdmaInt == FALSE);	
	g_bPdmaInt = FALSE;

	EDMA_Free(g_PdmaCh); 
	
	outp32(REG_SPI0_EDMA, inp32(REG_SPI0_EDMA) & ~EDMA_GO);
		
	spiSetByteEndin(0, eDRVSPI_DISABLE);
	spiTxLen(0, 0, 8);

	if(len % 4)
	{		
		ptr = buf + u32len;		
		
		for (i=0; i<(len %4); i++)
		{			
			outpb(REG_SPI0_TX0, 0xff);			
			spiActive(0);
			*ptr++ = inpb(REG_SPI0_RX0);
		}		
	}

	spiDisable(0);	// CS0
	
	return Successful;
}

void SPIFlashTest(void)
{
	unsigned char *pSrc, *pDst;
	int i;
	
	pSrc = (UINT8 *)((UINT32)WriteBuffer | NON_CACHE_BIT);
	pDst = (UINT8 *)((UINT32)ReadBuffer | NON_CACHE_BIT);

	for (i=0; i<TEST_SIZE; i++)
		*(pSrc+i) = i & 0xff;

	spiFlashInit();		

	usiCheckBusy();

	sysprintf("\tErase all SpiFlash\n");
	spiFlashEraseAll();

	sysprintf("\tWrite SpiFlash\n");	
	spiFlashPDMAWrite(0, TEST_SIZE, pSrc);	
		
	sysprintf("\tRead and Compare SpiFlash\n");	
	spiFlashPDMARead(0, TEST_SIZE, pDst);			

	for (i=0; i<TEST_SIZE; i++)
	{
		if (*(pSrc+i) != *(pDst+i))
		{
			sysprintf("error!! Src[%d] = 0x%X, Dst[%d] = 0x%X\n", i, *(pSrc+i), i, *(pDst+i));
			while(1);
//			break;
		}
	}

	sysprintf("finish SPI test\n");
	
}

INT spiFlashPDMAQuadWrite(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile count=0, page, i, j;
	UINT8 data;	
	PUINT8 ptr;	
	
	usiStatusWrite1(0x00, 0x02);	// clear block protect , set QE

	usiStatusRead(0x35, &data);
	sysprintf("SpiFlash Register-2 Status 0x%x \n", data);	
		
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

		spiEnable(0);	// CS0

		// write command
		outpw(REG_SPI0_TX0, 0x32);
		spiTxLen(0, 0, 8);
		spiActive(0);

		// address
		outpw(REG_SPI0_TX0, addr+i*256);
		spiTxLen(0, 0, 24);
		spiActive(0);

		outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);
					
		// write data
		if(page == 256)
		{
			spiSetByteEndin(0, eDRVSPI_ENABLE);
			spiTxLen(0, 0, 32);
		
			initSPIPDMA_Write((UINT32)buf, page);
			buf = buf + page;

			EDMA_Trigger(g_PdmaCh);
			outp32(REG_SPI0_EDMA, (inp32(REG_SPI0_EDMA) & ~0x03) | EDMA_GO);							

			while(g_bPdmaInt == FALSE);	
			g_bPdmaInt = FALSE;

			while(spiIsBusy(0));

			EDMA_Free(g_PdmaCh); 

			outp32(REG_SPI0_EDMA, inp32(REG_SPI0_EDMA) & ~EDMA_GO);
		}
		else
		{
			spiSetByteEndin(0, eDRVSPI_DISABLE);
			spiTxLen(0, 0, 8);
			
			ptr = buf;
			for (j=0; j<page; j++)
			{						
				outpw(REG_SPI0_TX0, *ptr);						
				spiActive(0);
				ptr++;						
			}			
		}
						
		spiDisable(0);	// CS0
		
		spiSetByteEndin(0, eDRVSPI_DISABLE);
		spiTxLen(0, 0, 8);		

		outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);

		// check status
		usiCheckBusy();	
		
	}	
	
	return Successful;
}

INT spiFlashPDMAQuadRead(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;
	PUINT8 ptr;	
	UINT8 data;
	UINT32 u32len;
		
	usiStatusWrite1(0x00, 0x02);	// clear block protect , set QE

	usiStatusRead(0x35, &data);
	sysprintf("SpiFlash Register-2 Status 0x%x \n", data);	

	spiEnable(0);	// CS0

	// read command
	outpw(REG_SPI0_TX0, 0x6B);
	spiTxLen(0, 0, 8);
	spiActive(0);

	// address
	outpw(REG_SPI0_TX0, addr);
	spiTxLen(0, 0, 24);
	spiActive(0);

	// dummy clock
	outpw(REG_SPI0_TX0, 0xFF);
	spiTxLen(0, 0, 8);
	spiActive(0);	
	
	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);

	spiSetByteEndin(0, eDRVSPI_ENABLE);
	spiTxLen(0, 0, 32);

	u32len = len;

	if(len % 4)
		u32len = (len/4)*4;

	initSPIPDMA_Read((UINT32)buf, u32len);
	
	EDMA_Trigger(g_PdmaCh);
	outp32(REG_SPI0_EDMA, (inp32(REG_SPI0_EDMA) & ~0x03) | (EDMA_RW | EDMA_GO));	
	outp32(REG_SPI0_CNTRL, inp32(REG_SPI0_CNTRL) |GO_BUSY);	    
	
	while(g_bPdmaInt == FALSE);	
	g_bPdmaInt = FALSE;	

	EDMA_Free(g_PdmaCh); 

	outp32(REG_SPI0_EDMA, inp32(REG_SPI0_EDMA) & ~EDMA_GO);

	spiSetByteEndin(0, eDRVSPI_DISABLE);
	spiTxLen(0, 0, 8);
	
	if(len % 4)
	{		
		ptr = buf + u32len;		
		
		for (i=0; i<(len %4); i++)
		{			
			outpb(REG_SPI0_TX0, 0xff);			
			spiActive(0);
			*ptr++ = inpb(REG_SPI0_RX0);
		}		
	}
	
	spiDisable(0);	// CS0
	
	outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);

	return Successful;
}

void SPIFlashQuadTest(void)
{
	unsigned char *pSrc, *pDst;
	int i;
	
	pSrc = (UINT8 *)((UINT32)WriteBuffer | NON_CACHE_BIT);
	pDst = (UINT8 *)((UINT32)ReadBuffer | NON_CACHE_BIT);

	for (i=0; i<TEST_SIZE; i++)
		*(pSrc+i) = i & 0xff;

	spiFlashInit();		

	usiCheckBusy();

	sysprintf("\tErase all SpiFlash\n");
	spiFlashEraseAll();

	sysprintf("\tWrite SpiFlash\n");		
	spiFlashPDMAQuadWrite(0, TEST_SIZE, pSrc);	

	sysprintf("\tRead and Compare SpiFlash\n");		
	spiFlashPDMAQuadRead(0, TEST_SIZE, pDst);	

	for (i=0; i<TEST_SIZE; i++)
	{
		if (*(pSrc+i) != *(pDst+i))
		{
			sysprintf("error!! Src[%d] = 0x%X, Dst[%d] = 0x%X\n", i, *(pSrc+i), i, *(pDst+i));
			while(1);
//			break;
		}
	}

	sysprintf("finish SPI test\n");
	
}








