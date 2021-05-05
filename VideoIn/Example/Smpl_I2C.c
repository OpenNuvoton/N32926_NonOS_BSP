/**************************************************************************//**
 * @file     Smpl_I2C.c
 * @brief    I2C middle layer for various I2C format
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "wblib.h"
#include "W55FA92_VideoIn.h"
#include "W55FA92_GPIO.h"
#include "DrvI2C.h"
#include "demo.h"

BOOL
I2C_Write_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr, UINT8 uData)
{
    // 3-Phase(ID address, regiseter address, data(8bits)) write transmission
    volatile int u32Delay = 0x100;
    DrvI2C_SendStart();
    while(u32Delay--);
    if ( (DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8)==FALSE) ||          // Write ID address to sensor
            (DrvI2C_WriteByte(uRegAddr,DrvI2C_Ack_Have,8)==FALSE) ||   // Write register address to sensor
            (DrvI2C_WriteByte(uData,DrvI2C_Ack_Have,8)==FALSE) )       // Write data to sensor
    {
        DrvI2C_SendStop();
        sysprintf("NACK\n");
        return FALSE;
    }
    DrvI2C_SendStop();

    if (uRegAddr==0x12 && (uData&0x80)!=0)
    {
        Delay(1000);
    }
    return TRUE;
}

UINT8 I2C_Read_8bitSlaveAddr_8bitReg_8bitData(UINT8 uAddr, UINT8 uRegAddr)
{
    UINT8 u8Data;

    // 2-Phase(ID address, register address) write transmission
    DrvI2C_SendStart();
    DrvI2C_WriteByte(uAddr,DrvI2C_Ack_Have,8);      // Write ID address to sensor
    DrvI2C_WriteByte(uRegAddr,DrvI2C_Ack_Have,8);   // Write register address to sensor
    DrvI2C_SendStop();

    // 2-Phase(ID-address, data(8bits)) read transmission
    DrvI2C_SendStart();
    DrvI2C_WriteByte(uAddr|0x01,DrvI2C_Ack_Have,8);     // Write ID address to sensor
    u8Data = DrvI2C_ReadByte(DrvI2C_Ack_Have,8);        // Read data from sensor
    DrvI2C_SendStop();

    return u8Data;
}


