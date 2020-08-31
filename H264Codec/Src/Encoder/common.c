/**************************************************************************//**
 * @file     common.c
 * @version  V3.00
 * @brief    N3292x series H.264 driver source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include "port.h"
#include "common.h"

/**
 ************************************************************************
 * \brief
 *    calculate Ceil(Log2(uiVal))
 ************************************************************************
 */
unsigned CeilLog2( unsigned uiVal)
{
  unsigned uiTmp = uiVal-1;
  unsigned uiRet = 0;

  while( uiTmp != 0 )
  {
    uiTmp >>= 1;
    uiRet++;
  }
  return uiRet;
}

