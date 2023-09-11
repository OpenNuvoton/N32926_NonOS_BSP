/**************************************************************************//**
 * @file     InterfaceCtl.h
 * @brief    Header file for NandWriter interface control.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#ifndef _INTERFACE_CTL_H_
#define _INTERFACE_CTL_H_

#define LED_ON  (0)     /* GPIO output value LED_ON  to turn on  LED */
#define LED_OFF (1)     /* GPIO output value LED_OFF to turn off LED */

void Interface_Init(void);
void Interface_LED_Y_Toggle(void);
void Interface_LED_Y_Stop(void);
void Interface_LED_G_ON(void);
void Interface_LED_G_OFF(void);
void Interface_LED_R_ON(void);
void Interface_LED_R_OFF(void);
void Interface_NAND_Power_ON(void);
void Interface_NAND_Power_OFF(void);
void Interface_Wait_Start(void);

#endif  // _INTERFACE_CTL_H_
