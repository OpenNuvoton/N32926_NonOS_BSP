/****************************************************************************
 * @file     SpiRead.c
 * @version  V1.00
 * $Revision: 4 $
 * $Date: 18/04/25 11:43a $
 * @brief    SpiLoader source file
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#include "wblib.h"

#define  SPICMD_DUMMY         0x00
#define  SPICMD_READ_DATA     0x03

#define CACHE_BIT31  BIT31

int spiActive(int port);
int spiTxLen(int port, int count, int bitLen);

void EDMAMasterRead(UINT32 u32DestAddr, UINT32 u32Length)
{
    UINT32 u32SFRCSR,u32Value,u32SPIBitLen;

    /* Set Byte Endian */
    outp32(REG_SPI0_CNTRL, inp32(REG_SPI0_CNTRL) | 1<<20);

    u32SPIBitLen=0;

    outp32(REG_SPI0_CNTRL, (inp32(REG_SPI0_CNTRL) & ~Tx_BIT_LEN) | u32SPIBitLen<<3);    

    u32SFRCSR = REG_PDMA_CSR1;

    outp32(REG_PDMA_SAR1, REG_SPI0_RX0);             /* EDMA Source */
    outp32(REG_PDMA_DAR1,(u32DestAddr| 0x80000000)); /* EDMA Destination */
    outp32(REG_PDMA_BCR1, u32Length);                /* EDMA Byte Count */
    u32Value = 0x00000005;                           /* Src:Fix, Dest:Inc, IP2Mem, EDMA enable */
    //u32Value = u32Value | (eTransferWidth<<19);    /* Change APB_TWS */

    outp32(u32SFRCSR,u32Value);                      /* EDMA Transer width */

    outp32(u32SFRCSR,inp32(u32SFRCSR) | TRIG_EN);    /* Enable EDMA Trig_En */

    outp32(REG_SPI0_EDMA,0x03);                      /* Enable SPI EDMA Read & Start */
    outp32(REG_SPI0_CNTRL, inp32(REG_SPI0_CNTRL) |1);    /* master start */
    
    while(inp32(u32SFRCSR) & TRIG_EN);
    
}

int SPIReadFast(BOOL bEDMAread, UINT32 addr, UINT32 len, UINT32 *buf)
{
    int volatile i;
    UINT32 u32Tmp,u32Count;
    //sysprintf("Load file length 0x%x, execute address 0x%x\n", len, (UINT32)buf);

    outp32(REG_SPI0_CNTRL, inp32(REG_SPI0_CNTRL) & ~(1<<20));    /* disabe BYTE endian */

    outp32(REG_SPI0_SSR, inp32(REG_SPI0_SSR) | 0x01);    /* CS0 */

    buf = (UINT32 *) ((UINT32)buf | (UINT32)CACHE_BIT31);

    /* read command */
    outp32(REG_SPI0_TX0, 0x0B);
    spiTxLen(0, 0, 8);
    spiActive(0);

    /* address */
    outp32(REG_SPI0_TX0, addr);
    spiTxLen(0, 0, 24);
    spiActive(0);

    /* dummy byte */
    outp32(REG_SPI0_TX0, 0xff);
    spiTxLen(0, 0, 8);
    spiActive(0);
    if(bEDMAread)
    {
        EDMAMasterRead((UINT32)buf, len);
    }
    else
    {
        u32Count = len/4;
        if(len % 4)
            u32Count++;

        /* data */
        for (i=0; i<u32Count; i++)
        {
            outp32(REG_SPI0_TX0, 0xffffffff);
            spiTxLen(0, 0, 32);
            spiActive(0);
            u32Tmp = inp32(REG_SPI0_RX0);
            *buf++ = ((u32Tmp & 0xFF) << 24) | ((u32Tmp & 0xFF00) << 8) | ((u32Tmp & 0xFF0000) >> 8)| ((u32Tmp & 0xFF000000) >> 24);
        }
    }
    outp32(REG_SPI0_SSR, inp32(REG_SPI0_SSR) & 0xfe);    /* CS0 */

    return 0;
}

#ifdef __OTP_4BIT__

static     UINT32 g_u32JEDEC = 0;

typedef enum
{
    eDRVSPI_DISABLE =0,
    eDRVSPI_ENABLE
}E_DRVSPI_OPERATION;

typedef enum
{
    eDRVSPI_DIRECTION_OUTPUT = 0,
    eDRVSPI_DIRECTION_INPUT = 0x1000000    
}E_DRVSPI_DIRECTION_SELECT;

typedef enum
{
    eDRVSPI_1BITS = 0,
    eDRVSPI_2BITS = 0x100,
    eDRVSPI_4BITS = 0x200
}E_DRVSPI_BITS_MODE_SELECT;

/*-----------------------------------------------------------------------------------*/
VOID spiSetByteEndin(E_DRVSPI_OPERATION eOP)
{
    if(eOP == eDRVSPI_ENABLE)
        outpw(REG_SPI0_CNTRL, inpw(REG_SPI0_CNTRL) |BYTE_ENDIN);
    else
        outpw(REG_SPI0_CNTRL, inpw(REG_SPI0_CNTRL) & ~BYTE_ENDIN);
}

int usiSendSPIMsg( UINT8* u8Data, UINT8 u8DataLen )
{
    int i;

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);  /* CS0 */

    if ( u8Data )
    {
        for (i=0;i<u8DataLen;i++)
        {
            outpw(REG_SPI0_TX0, u8Data[i]&0xff);
            spiTxLen(0, 0, 8);
            spiActive(0);
        }
    }
    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);  /* CS0 */

    return Successful;
}

/*
    This for Winbond QPI read
    CONFIG_SPIFLASH_W25Q256FV
*/
INT WB_spiFlashFastReadQuads(UINT32 addr, UINT32 len, UINT32* buf)
{
    int volatile i,j;
    PUINT8 ptr;
    PUINT32 p32tmp;
    UINT8 cmdbuf[8];
    UINT32 transfer_len;

    p32tmp = buf;    

    /* Set WE */
    cmdbuf[0]=0x06;
    usiSendSPIMsg (cmdbuf, 1);

    /* Set QE */
    cmdbuf[0]=0x31;
    cmdbuf[1]=0x00;
    cmdbuf[2]=0x02;
    usiSendSPIMsg (cmdbuf, 3); 

    /* check status */
    usiCheckBusy();

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);  /* CS0 */

    /* read command */
    outpw(REG_SPI0_TX0, 0x6B);
    spiTxLen(0, 0, 8);
    spiActive(0);

    /* address */
    outpw(REG_SPI0_TX0, addr);
    spiTxLen(0, 0, 24);
    spiActive(0);

    /* dummy clock */
    outpw(REG_SPI0_TX0, 0xFF);
    spiTxLen(0, 0, 8);
    spiActive(0);

    outp32(REG_SPI0_DIVIDER, 0x1);
    outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);

    spiSetByteEndin(eDRVSPI_ENABLE);
    spiTxLen(0, 7, 32);    /* multi-transfer */

    for(j=0; j<8; j++)
        outpw(REG_SPI0_TX0 + 4*j, 0xffffffff);

    /* data */
    transfer_len = len/32;
    for (i=0; i<transfer_len; i++)
    {
        spiActive(0);
        for(j=0; j<8; j++)
            *p32tmp++ = inpw(REG_SPI0_RX0 + 4*j);
    }

    spiSetByteEndin(eDRVSPI_DISABLE);
    spiTxLen(0, 0, 8);    /* one-transfer */

    if(len % 32)
    {
        ptr = (PUINT8)p32tmp;
        transfer_len = len %32;
        for (i=0; i<transfer_len; i++)
        {
            outpb(REG_SPI0_TX0, 0xff);
            spiActive(0);
            *ptr++ = inpb(REG_SPI0_RX0);
        }
    }

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);  /* CS0 */

    outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);
    outp32(REG_SPI0_DIVIDER, 0x1);

    /* Set QE */
    cmdbuf[0]=0x01;
    cmdbuf[1]=0x00;
    cmdbuf[2]=0x00;
    usiSendSPIMsg (cmdbuf, 3);

    /* check status */
    usiCheckBusy();

    return Successful;
}

/*
    This for MXIC/EON QPI read
    CONFIG_SPIFLASH_MX25L256
    CONFIG_SPIFLASH_EN25QH256
*/
INT MXIC_spiFlashFastReadQuads(UINT32 addr, UINT32 len, UINT32* buf)
{
    int volatile i,j;
    PUINT8 ptr;
    PUINT32 p32tmp;
    UINT8 cmdbuf[8];
    UINT32 transfer_len;

    p32tmp = buf;    
    
    /* Set WE */
    cmdbuf[0]=0x06;
    usiSendSPIMsg (cmdbuf, 1);
    
    /* Set QE */
    cmdbuf[0]=0x01;
    cmdbuf[1]=0x40;
    usiSendSPIMsg (cmdbuf, 2); 

    /* check status */
    usiCheckBusy();

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);    /* CS0 */

    /* read command */
    outpw(REG_SPI0_TX0, 0xEB);
    spiTxLen(0, 0, 8);
    spiActive(0);

    outp32(REG_SPI0_DIVIDER, 0x1);
    outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_OUTPUT |eDRVSPI_4BITS);

    /* address */
    outpw(REG_SPI0_TX0, addr);
    spiTxLen(0, 0, 24);
    spiActive(0);

    /* dummy clock */
    outpw(REG_SPI0_TX0, 0xFFFFFF);
    spiTxLen(0, 0, 24);
    spiActive(0);

    outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(SIO_DIR|BIT_MODE)) |eDRVSPI_DIRECTION_INPUT |eDRVSPI_4BITS);
    
    spiSetByteEndin(eDRVSPI_ENABLE);
    spiTxLen(0, 7, 32);    /* multi-transfer */
    
        for(j=0; j<8; j++)
            outpw(REG_SPI0_TX0 + 4*j, 0xffffffff);
    /* data */
    transfer_len = len/32;
    for (i=0; i<transfer_len; i++)
    {                    
        spiActive(0);
        for(j=0; j<8; j++)
            *p32tmp++ = inpw(REG_SPI0_RX0 + 4*j);
    }
    
    spiSetByteEndin(eDRVSPI_DISABLE);
    spiTxLen(0, 0, 8);    /* one-transfer */

    if(len % 32)
    {
        ptr = (PUINT8)p32tmp;
        transfer_len = len %32;
        for (i=0; i<transfer_len; i++)
        {            
            outpb(REG_SPI0_TX0, 0xff);
            spiActive(0);
            *ptr++ = inpb(REG_SPI0_RX0);
        }        
    }

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);    /* CS0 */
    
    outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);
    outp32(REG_SPI0_DIVIDER, 0x1);

    /* Disable QE */
    cmdbuf[0]=0x01;
    cmdbuf[1]=0x0;
    usiSendSPIMsg (cmdbuf, 2); 

    /* check status */
    usiCheckBusy();

    return Successful;
}

INT spiFlashFastReadQuads(UINT32 addr, UINT32 len, UINT32* buf)
{
    switch( g_u32JEDEC>>16 )
    {    
            case 0xef:
            case 0xc8:
                    //sysprintf("Using WB/GIGA Quad SPI to read\n");
                    return WB_spiFlashFastReadQuads(addr, len, buf);
            case 0xc2:            
            case 0x1c:
                    //sysprintf("Using MXIC/EON Quad SPI to read\n");
                    return MXIC_spiFlashFastReadQuads(addr, len, buf);
            default:
                    //sysprintf("Using 1-bit SPI to read\n");
                    return SPIReadFast(0, addr, len, buf);
                    break;    
    }
    
    return -1;
}


void JEDEC_Probe (void)
{    
    outpw(REG_SPI0_CNTRL, (inpw(REG_SPI0_CNTRL) & ~(BIT_MODE)) |eDRVSPI_1BITS);

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) | 0x01);    /* CS0 */

    /* command 8 bit */
    outpw(REG_SPI0_TX0, 0x9F);
    spiTxLen(0, 0, 8);
    spiActive(0);
    /* data 24 bit */
    outpw(REG_SPI0_TX0, 0xFFFFFFFF);
    spiTxLen(0, 0, 24);
    spiActive(0);

    g_u32JEDEC = inpw(REG_SPI0_RX0) & 0xffffff;

    outpw(REG_SPI0_SSR, inpw(REG_SPI0_SSR) & 0xfe);    /* CS0 */

    sysprintf("JEDEC ID: 0x%X \n",g_u32JEDEC );
}

#endif
VOID SPI_OpenSPI(void)
{
    outp32(REG_APBCLK, inp32(REG_APBCLK) | SPIMS0_CKE);    
    outp32(REG_GPDFUN1, (inp32(REG_GPDFUN1) & ~(MF_GPD15|MF_GPD14|MF_GPD13|MF_GPD12)) | 0x22220000);
    
    outp32(REG_SPI0_DIVIDER, 0x01);
}

VOID SPI_CloseSPI(void)
{
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~SPIMS0_CKE);
    /* Disable Pin function */
    outp32(REG_GPDFUN1, (inp32(REG_GPDFUN1) & ~(MF_GPD15|MF_GPD14|MF_GPD13|MF_GPD12)));        

}
