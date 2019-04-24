/***************************************************************************
 *                                                                         *
 * Copyright (c) 2016 Nuvoton Technolog. All rights reserved.              *
 *                                                                         *
 ***************************************************************************/

/****************************************************************************
 * FILENAME
 *   wb_huart.c
 *
 * VERSION
 *   1.0
 *
 * DESCRIPTION
 *   The HUART related function of Nuvoton ARM9 MCU
 *
 * HISTORY
 *   2008-06-25  Ver 1.0 draft by Min-Nan Cheng
 *
 * REMARK
 *   None
 **************************************************************************/
#include <string.h>
#include <stdio.h>
#include "wblib.h"

extern PFN_SYS_UART_CALLBACK (pfnUartIntHandlerTable)[2][2];    /* defined on wb_uart.c */

UINT32 _sys_uHUARTClockRate = EXTERNAL_CRYSTAL_CLOCK;
PVOID  _sys_pvOldHuartVect;

#define sysHuartTxBufReadNextOne()	(((_sys_uHuartTxHead+1)==HUART_BUFFSIZE)? NULL: _sys_uHuartTxHead+1)
#define sysHuartTxBufWriteNextOne()	(((_sys_uHuartTxTail+1)==HUART_BUFFSIZE)? NULL: _sys_uHuartTxTail+1)
#define HUART_BUFFSIZE	256
UINT8 _sys_ucHuartTxBuf[HUART_BUFFSIZE];
UINT32 volatile _sys_uHuartTxHead, _sys_uHuartTxTail;

#define RX_ARRAY_NUM 100
UINT8 huart_rx[RX_ARRAY_NUM] = {0};
UINT32 volatile huart_rx_cnt = 0;


void sysHuartInstallcallback(UINT32 u32IntType, PFN_SYS_UART_CALLBACK pfnCallback)
{
    if(u32IntType>1)
        return;
    if(u32IntType == 0)
    {
        pfnUartIntHandlerTable[0][0] = (PFN_SYS_UART_CALLBACK)(pfnCallback);
    }
    else if(u32IntType == 1)
    {
        pfnUartIntHandlerTable[0][1] = (PFN_SYS_UART_CALLBACK)(pfnCallback);
    }
}


VOID sysHuartISR()
{
    UINT32 volatile regIIR, i;
    UINT32 volatile regIER, u32EnableInt;
    UINT32 u32Count;

    regIIR = inpb(REG_UART_ISR + 0);
    regIER = inpb(REG_UART_IER + 0);
    u32EnableInt = regIER & regIIR;
    if (u32EnableInt & THRE_IF)
    {   /* buffer empty */
        if (_sys_uHuartTxHead == _sys_uHuartTxTail)
        {
            /* Disable interrupt if no any request! */
            outpb((REG_UART_IER+0), inp32(REG_UART_IER+0) & (~THRE_IEN));
        }
        else
        {
            /* Transmit data */
            for (i=0; i<8; i++)
            {
                #ifdef __HW_SIM__

                #else
                    outpb(REG_UART_THR+0, _sys_ucHuartTxBuf[_sys_uHuartTxHead]);
                #endif
                _sys_uHuartTxHead = sysHuartTxBufReadNextOne();
                if (_sys_uHuartTxHead == _sys_uHuartTxTail) /* buffer empty */
                    break;
            }
        }
    }
    if(u32EnableInt & RDA_IF)
    {
        u32Count = (inpw(REG_UART_FSR+0) & Rx_Pointer) >> 8;
        for(i=0; i<u32Count; i++)
        {
            if ( huart_rx_cnt == RX_ARRAY_NUM )
            {
                huart_rx_cnt = 0;
            }
            huart_rx[huart_rx_cnt] = (inpb(REG_UART_RBR+0));
            huart_rx_cnt++;
        }
        /* callback to uplayer(bufptr, len); */
        if(pfnUartIntHandlerTable[0][0] !=0)
            pfnUartIntHandlerTable[0][0](&(huart_rx[0]), u32Count);
        huart_rx_cnt = 0;
    }
    if(u32EnableInt & Tout_IF)
    {
        u32Count = (inpw(REG_UART_FSR+0) & Rx_Pointer) >> 8;
        for(i=0; i<u32Count; i++)
        {
            if ( huart_rx_cnt == RX_ARRAY_NUM )
            {
                huart_rx_cnt = 0;
            }
            huart_rx[huart_rx_cnt] = (inpb(REG_UART_RBR+0));
            huart_rx_cnt++;
        }
        /* callback to uplayer(bufptr, len); */
        if(pfnUartIntHandlerTable[0][1] !=0)
            pfnUartIntHandlerTable[0][1](&(huart_rx[0]), u32Count);
        huart_rx_cnt = 0;
    }
}


static VOID sysSetHuartBaudRate(UINT32 uBaudRate)
{
    UINT32 _mBaudValue;

    /* First, compute the baudrate divisor. */
#if 0
    /* mode 0 */
    _mBaudValue = (_sys_uHUARTClockRate / (uBaudRate * 16));
    if ((_sys_uHUARTClockRate % (uBaudRate * 16)) > ((uBaudRate * 16) / 2))
        _mBaudValue++;
    _mBaudValue -= 2;
    outpw(REG_UART_BAUD+0, _mBaudValue);
#else
    /* mode 3 */
    _mBaudValue = (_sys_uHUARTClockRate / uBaudRate) - 2;
    outpw(REG_UART_BAUD+0, ((0x30<<24)| _mBaudValue));
#endif
}


void sysHuartEnableInt(INT32 eIntType)
{
    if(eIntType==UART_INT_RDA)
        outp32(REG_UART_IER+0, inp32(REG_UART_IER+0) |RDA_IEN);
    else if (eIntType==UART_INT_RDTO)
        outp32(REG_UART_IER+0, inp32(REG_UART_IER+0) |RTO_IEN |Time_out_EN);
    else if (eIntType==UART_INT_NONE)
        outp32(REG_UART_IER+0, 0x0);
}


INT32 sysInitializeHUART(WB_UART_T *uart)
{
    static BOOL bIsResetFIFO = FALSE;

    /* Enable HUART multi-function pins*/
	outp32(REG_CLKDIV3, (inp32(REG_CLKDIV3) & (~(UART0_N1| UART0_S| UART0_N0))));
	outp32(REG_GPDFUN0, (inp32(REG_GPDFUN0) & ~(MF_GPD1|MF_GPD2)) | 0x110);	
	outp32(REG_APBCLK, inp32(REG_APBCLK) | UART0_CKE);

    /* Check the supplied parity */
    if ((uart->uiParity != WB_PARITY_NONE) &&
        (uart->uiParity != WB_PARITY_EVEN) &&
        (uart->uiParity != WB_PARITY_ODD))

            /* The supplied parity is not valid */
            return (INT32)WB_INVALID_PARITY;

    /* Check the supplied number of data bits */
    else if ((uart->uiDataBits != WB_DATA_BITS_5) &&
             (uart->uiDataBits != WB_DATA_BITS_6) &&
             (uart->uiDataBits != WB_DATA_BITS_7) &&
             (uart->uiDataBits != WB_DATA_BITS_8))

            /* The supplied data bits value is not valid */
            return (INT32)WB_INVALID_DATA_BITS;

    /* Check the supplied number of stop bits */
    else if ((uart->uiStopBits != WB_STOP_BITS_1) &&
             (uart->uiStopBits != WB_STOP_BITS_2))

            /* The supplied stop bits value is not valid */
            return (INT32)WB_INVALID_STOP_BITS;

    /* Verify the baud rate is within acceptable range */
    else if (uart->uiBaudrate < 1200)
            /* The baud rate is out of range */
            return (INT32)WB_INVALID_BAUD;

    /* Reset the TX/RX FIFOs */
    if(bIsResetFIFO==FALSE)
    {
        outpw(REG_UART_FCR+0, 0x07);
        bIsResetFIFO = TRUE;
    }
    /* Setup reference clock */
    _sys_uHUARTClockRate = uart->uiFreq;

    /* Setup baud rate */
    sysSetHuartBaudRate(uart->uiBaudrate);

    /* Set the modem control register. Set DTR, RTS to output to LOW,
       and set INT output pin to normal operating mode */
    /* outpb(REG_UART_MCR, (WB_DTR_Low | WB_RTS_Low | WB_MODEM_En)); */

    /* Setup parity, data bits, and stop bits */
    outpw(REG_UART_LCR+0, (uart->uiParity | uart->uiDataBits | uart->uiStopBits));

    /* Timeout if more than ??? bits xfer time */
    outpw(REG_UART_TOR+0, 0x80+0x20);

    /* Setup Fifo trigger level and enable FIFO */
    outpw(REG_UART_FCR+0, (uart->uiRxTriggerLevel << 4) | 0x01);

    /* Enable HUART interrupt Only (Receive Data Available Interrupt & RX Time out Interrupt) */
#if 0
#if 1   /* Enable RX data time out */
    outp32(REG_UART_IER+0, inp32(REG_UART_IER+0) |RDA_IEN |RTO_IEN |Time_out_EN);
#else
    outp32(REG_UART_IER+0, inp32(REG_UART_IER+0) |RDA_IEN);
#endif
#endif

    /* Timeout if more than ??? bits xfer time */
//    outpw(REG_UART_TOR+0, 0x7F);
    outpw(REG_UART_TOR+0, 0x20);

    /* hook HUART interrupt service routine */
    _sys_uHuartTxHead = _sys_uHuartTxTail = NULL;
    _sys_pvOldHuartVect = sysInstallISR(IRQ_LEVEL_1, IRQ_HUART, (PVOID)sysHuartISR);
    sysEnableInterrupt(IRQ_HUART);

    return Successful;
}


INT8 sysHuartReceive()
{
    while (1)
    {
        if (inpw(REG_UART_ISR+0) & RDA_IF)
            return (inpb(REG_UART_RBR+0));
    }
}


void sysHuartTransfer(char* pu8buf, UINT32 u32Len)
{
    do
    {
        if( (inp32(REG_UART_FSR+0) & Tx_Full) == 0 )
        {
            outpb(REG_UART_THR+0, *pu8buf++);
            u32Len = u32Len - 1;
        }
    } while(u32Len != 0) ;
}
