/**************************************************************************//**
 * @file     DrvTouch.c
 * @version  V3.00
 * @brief    N3292x series TouchADC driver source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "wblib.h"
#include "W55FA92_ADC.h"


#define E_ADC_NONE              0
#define E_ADC_TP_WAIT               1
#define E_ADC_TP_GETXY              2
#define E_ADC_TP_GETZ               3
#define E_ADC_VD                    4
#define E_ADC_KEY_WAIT              5
#define E_ADC_KEY                   6

//static UINT16 g_Xpos, g_Ypos;

static UINT16 g_Z1press, g_Z2press;
static volatile UINT16 g_KeyCode;
static volatile UINT16 g_Voltage;

volatile UINT32 u32AdcMode = E_ADC_NONE;

static PFN_ADC_CALLBACK (g_psAdcCallBack)[5] = {0};
//PFN_VIDEOIN_CALLBACK (pfnVideoInIntHandlerTable)[2][4]={0};
// 0 KEY
// 1 Pen down
// 2 Normal ADC Conversion
// 4 Touch XY Position
// 5 Touch Z Position

INT32 DrvADC_InstallCallback(E_ADC_INT_TYPE eIntType,
                             PFN_ADC_CALLBACK pfnCallback,
                             PFN_ADC_CALLBACK* pfnOldCallback)
{
    if( (eIntType> (E_ADC_INT_TYPE)eADC_PRESSURE) ) //||  (eIntType< (E_ADC_INT_TYPE)eADC_KEY))
        return -1;
    pfnOldCallback = &(g_psAdcCallBack[eIntType]);
    g_psAdcCallBack[eIntType] = pfnCallback;
    return Successful;
}
#define DBG_PRINTF(...)
//#define DBG_PRINTF sysprintf

static void adcSetOperationMode(UINT32 u32Mode)
{
    BOOL bIsIBitEnable = sysGetIBitState();
    if(bIsIBitEnable==TRUE)
        sysSetLocalInterrupt(DISABLE_IRQ);

    u32AdcMode = u32Mode;

    if(bIsIBitEnable==TRUE)
        sysSetLocalInterrupt(ENABLE_IRQ);
}
static UINT32 adcGetOperationMode(void)
{
    return u32AdcMode;
}

extern INT32 DrvADC_DisableInt(E_ADC_INT_TYPE eIntType);
extern INT32 DrvADC_EnableInt(E_ADC_INT_TYPE eIntType);

static UINT32 u32ValidPosition;
static UINT32 u32ValidPressure;
static void AdcIntHandler(void)
{
    UINT32 u32IntStatus = inp32(REG_TP_INTST);
    UINT32 u32OperationMode = adcGetOperationMode();
#if 0
    if((u32IntStatus &INT_KEY) ==INT_KEY) //Key
    {
        DBG_PRINTF("Keypad Int\n");
        if(u32OperationMode == E_ADC_KEY_WAIT)
        {
            DBG_PRINTF("Keypad down\n");
            DrvADC_DisableInt(eADC_KEY);
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_TC) ) | INT_KEY); /* Write one clear */
            /* Force ADC to convese key code */
            adcSetOperationMode(E_ADC_KEY);
            DrvADC_EnableInt(eADC_AIN);
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | SW_GET);
        }
        else if (u32OperationMode == E_ADC_KEY)
        {
            DBG_PRINTF("Get Keypad value\n");
            g_psAdcCallBack[0](inp32(REG_NORM_DATA));    /* Notice Upper lay key pressing */
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_TC) ) | INT_KEY);  /* Write one clear */
            DrvADC_DisableInt(eADC_AIN);
            adcSetOperationMode(E_ADC_NONE);
        }
    }
#endif
    if((u32IntStatus &INT_TC) ==INT_TC) //Touch
    {
        if(u32OperationMode == E_ADC_NONE)
        {
            DBG_PRINTF("Touch down\n");
            DrvADC_DisableInt(eADC_TOUCH);
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY)) | INT_TC);  /* Write one clear */

            adcSetOperationMode(E_ADC_TP_GETXY);
            DrvADC_EnableInt(eADC_AIN);
            /* Force ADC to get (X, Y) Position */
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | GET_XY);
        }
    }
    if((u32IntStatus &INT_NOR) ==INT_NOR) //AIN
    {
        DBG_PRINTF("Normal Int\n");
        if(u32OperationMode == E_ADC_TP_GETXY)
        {
            u32ValidPosition =  inp32(REG_XY_DATA);
            if( (u32ValidPosition&(BIT31|BIT15)) == (BIT31|BIT15) )
            {
                adcSetOperationMode(E_ADC_TP_GETZ);
                /* Force ADC to get Z Pressure */
                outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | GET_PRESSURE);
            }
            else
            {
                u32ValidPosition = 0;
                adcSetOperationMode(E_ADC_NONE);
                outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)));
            }
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST)&~(INT_NOR|INT_KEY)) | INT_NOR);   /* Write one clear */
        }
        else if(u32OperationMode == E_ADC_TP_GETZ)
        {
            g_Z1press = inp32(REG_Z_DATA) >>16;
            g_Z2press = inp32(REG_Z_DATA) & 0xFFFF;
            u32ValidPressure = inp32(REG_Z_DATA);

            if( (u32ValidPressure &(BIT31|BIT15)) == (BIT31|BIT15) )
            {
                if( g_psAdcCallBack[1]!=0)
                    g_psAdcCallBack[1](1);  /* Notice Upper lay XY Position */
                if( g_psAdcCallBack[3]!=0)
                    g_psAdcCallBack[3](u32ValidPosition);  /* Notice Upper lay XY Position */
                if( g_psAdcCallBack[4]!=0)
                    g_psAdcCallBack[4](u32ValidPressure);   /* Notice Upper lay Z Pressure */
            }
            else
            {
                //u32ValidPressure = 0;
                if( g_psAdcCallBack[1]!=0)
                    g_psAdcCallBack[1](0);  /* Notice Upper lay XY Position */
            }
            adcSetOperationMode(E_ADC_NONE);
            /* Force ADC to None */
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)));
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST)&~(INT_NOR|INT_KEY)) | INT_NOR);   /* Write one clear */
        }
        else if (u32OperationMode == E_ADC_VD)
        {
            DBG_PRINTF("Get Battery value\n");
            g_Voltage = inp32(REG_NORM_DATA);
            if( g_psAdcCallBack[2]!=0)
                g_psAdcCallBack[2](g_Voltage);      /* Notice Upper lay voltage detect */

            adcSetOperationMode(E_ADC_NONE);
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST)&~(INT_NOR|INT_KEY)) | INT_NOR); /* Write one clear */
        }
        else if (u32OperationMode == E_ADC_KEY)
        {
            DBG_PRINTF("Get Keypad value\n");
            g_KeyCode =  inp32(REG_NORM_DATA);
            if( g_psAdcCallBack[0]!=0)
                g_psAdcCallBack[0](inp32(REG_NORM_DATA));      /* Notice Upper lay key pressing */
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_TC) ) | INT_KEY);  /* Write one clear */
            DrvADC_DisableInt(eADC_AIN);
            adcSetOperationMode(E_ADC_NONE);
        }
    }
    if(u32OperationMode == E_ADC_NONE )
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST))  | (INT_KEY|INT_NOR | INT_TC));  /* Alway clear all interrupt status if ADC free */
}
#define REAL_CHIP
INT32 DrvADC_Open(void)
{
    //UINT8 u8RegDac;
    /* Enable Clock */
    outp32(REG_APBCLK, inp32(REG_APBCLK) | TOUCH_CKE);
    /* Specified Clock */
#ifdef  REAL_CHIP
    outp32(REG_CLKDIV5, (inp32(REG_CLKDIV5) & ~(TOUCH_N1 | TOUCH_S| TOUCH_N0)) | (2<<27) );     /* Fed to ADC clock need 12MHz=External clock */
#else
    outp32(REG_CLKDIV5, (inp32(REG_CLKDIV5) & ~(TOUCH_N1 | TOUCH_S| TOUCH_N0)) | (2<<27) );/* FPGA: Divider need to be 1 at least for I2C clock*/
#endif
    /* IP Reset */
    outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TOUCHRST);
    outp32(REG_APBIPRST, inp32(REG_APBIPRST)  & ~TOUCHRST);

    outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) & ~(PD_Power|PD_BUF| ADC_SLEEP) ); //| PD_BUF); //If REF_SEL!=0 PD_BUF==>1
    outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | REF_SEL);      //Vref reference voltage

    outp32(REG_TP_CTL2, 0xFF);  /* Delay 255 ADC */
    sysInstallISR(IRQ_LEVEL_1,
                  IRQ_TOUCH,
                  (PVOID)AdcIntHandler);
    sysEnableInterrupt(IRQ_TOUCH);
    return Successful;
}
INT32 DrvADC_Channel(UINT32 u32Channel)
{
    if(u32Channel>7)
        return -1;
    outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~IN_SEL) | (u32Channel<<16));
    return Successful;
}

INT32 DrvADC_EnableInt(E_ADC_INT_TYPE eIntType)
{
    if( (eIntType>eADC_AIN) ||  (eIntType<eADC_KEY))
        return -1;
    outp32(REG_TP_INTST, ((inp32(REG_TP_INTST) & ~(INT_NOR|INT_TC|INT_KEY)) |  ((1<<eIntType)<<4)) );
    return Successful;
}
INT32 DrvADC_DisableInt(E_ADC_INT_TYPE eIntType)
{
    if( (eIntType>eADC_AIN) ||  (eIntType<eADC_KEY))
        return -1;
    outp32(REG_TP_INTST, (inp32(REG_TP_INTST)  & ~(INT_NOR|INT_TC|INT_KEY) ) &  ~((1<<eIntType)<<4)  ) ;
    return Successful;
}

INT32 DrvADC_Close(void)
{
    outp32(REG_APBCLK, inp32(REG_APBCLK) & ~ TOUCH_CKE);
    return Successful;
}
/*
    The function should be called every 20ms.
*/
void TouchDelay(UINT32 u32Dly)
{
    //In CPU 162MHz. The u32Dly will be 1.
    volatile UINT32 u32Delay;
    for(u32Delay =0; u32Delay<(1*(u32Dly+1)); u32Delay=u32Delay+1);
}
UINT32 TouchDelayCheckPenDown(UINT32 u32Dly)
{
    //In CPU 162MHz. The u32Dly will be 1.
    volatile UINT32 u32Delay;
    for(u32Delay =0; u32Delay<(1*(u32Dly+1)); u32Delay=u32Delay+1)
    {
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY)) | INT_TC);
        TouchDelay(1);
        if((inp32(REG_TP_INTST)&INT_TC)!=INT_TC)
        {
            return 0;   //Pen up
        }
    }
    return 1;   //Pen down
}
INT32 DrvADC_PenDetection(BOOL bIs5Wire)
{
    if(adcGetOperationMode() == E_ADC_NONE)
    {
        adcSetOperationMode(E_ADC_TP_WAIT);
        if(bIs5Wire==1)
            outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  | TSMODE);
        else
            outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  & ~TSMODE);
        outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  | IN_SEL);
        outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(XP_EN | XM_EN | YP_EN)) | (YM_EN| PLLUP) );

        TouchDelay(1);          /* Due to PLLUP bit cause the INT_TC was set. */
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) &~(INT_NOR|INT_KEY) ) | INT_TC);  /* Write one clear */
        //DrvADC_EnableInt(eADC_TOUCH);
        TouchDelay(1);

        if(inp32(REG_TP_INTST)&INT_TC)
        {
            DrvADC_DisableInt(eADC_TOUCH);

            //outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~PLLUP) );

            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY) ) | INT_TC); /* Write one clear */
            /* Force ADC to convese key code */
            adcSetOperationMode(E_ADC_TP_GETXY);
            DrvADC_EnableInt(eADC_AIN);
            //outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | SW_GET);
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | GET_XY);
        }
        else
        {
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~PLLUP) );
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY) ) | INT_TC); /* Write one clear */
            adcSetOperationMode(E_ADC_NONE);
            return E_TOUCH_UP;
        }

        return Successful;
    }
    return E_ADC_BUSY;
}


void DrvADC_Wakeup(E_ADC_WAKEUP_TYPE eWakeupSrc)
{
    if(eWakeupSrc&eADC_WAKEUP_TOUCH)
    {
        outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(XP_EN | XM_EN | YP_EN)) | (YM_EN| PLLUP) );

        TouchDelay(1);          /* Due to PLLUP bit cause the INT_TC was set. */
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) &~(INT_NOR|INT_KEY) ) | INT_TC);  /* Write one clear */
    }
    else
    {
        outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(XP_EN | XM_EN | YP_EN)) );
    }
    if(eWakeupSrc&eADC_WAKEUP_KEY)
    {
        outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) & ~PD_Power ); //Not to Power down key pad
    }
    else
    {
        outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | PD_Power );    //Power down key pad
    }
}
/*
    The function should be called every 20ms.
*/
static UINT32 u32RepeatedlyKey = 0;
static UINT32 u32LastKey = 0;
#define MAX_KEY_ARRY  5
INT32 DrvADC_KeyDetection(UINT32 u32Channel, UINT32* pu32KeyCode)
{
#if 1
    UINT32 u32Idx;
    UINT32 u32KeyCode[MAX_KEY_ARRY]= {0};
    if(adcGetOperationMode() == E_ADC_NONE)
    {
        adcSetOperationMode(E_ADC_KEY_WAIT);
        DrvADC_Channel(u32Channel);
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) | INT_KEY));  /* Write one clear */
        //DrvADC_EnableInt(eADC_KEY);
        if( (inp32(REG_TP_INTST) & INT_KEY)  == INT_KEY) /* Key press */
        {

            for(u32Idx=0; u32Idx<MAX_KEY_ARRY; u32Idx=u32Idx+1)
            {

                adcSetOperationMode(E_ADC_KEY_WAIT);
                outp32(REG_TP_INTST, (inp32(REG_TP_INTST) | INT_KEY));  /* Write one clear */
                if( (inp32(REG_TP_INTST) & INT_KEY)  != INT_KEY)
                {
                    adcSetOperationMode(E_ADC_NONE);
                    u32LastKey = 0;
                    u32RepeatedlyKey = 0;
                    return E_KEYPAD_UP;
                }

                g_KeyCode = 0xFFFF; /* 12 bits ADC, ADC value can't report up to 0xFFFF */
                adcSetOperationMode(E_ADC_KEY);
                DrvADC_EnableInt(eADC_AIN);
                outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | SW_GET);
                while(g_KeyCode==0xFFFF);
                u32KeyCode[u32Idx] = keymap(g_KeyCode);
            }
#if 0
            for(u32Idx=1; u32Idx<MAX_KEY_ARRY; u32Idx=u32Idx+1)
            {
                if(u32KeyCode[0] != u32KeyCode[u32Idx])
                {
                    u32LastKey = 0;
                    u32RepeatedlyKey = 0;
                    return E_KEYPAD_UP;
                }
            }
#else
            {
                UINT32 u32CheckTime = 0;
                UINT32 u32CheckKeyCode = 0;
                UINT32 u32TraceKey = 0;
                for(u32Idx=0; u32Idx<MAX_KEY_ARRY; u32Idx=u32Idx+1)
                {
                    if(u32KeyCode[u32Idx]!=0)
                    {
                        if(u32TraceKey==0)
                            g_KeyCode = u32KeyCode[u32Idx];
                        else
                        {
                            if( g_KeyCode != u32KeyCode[u32Idx] )
                            {
                                u32LastKey = 0;
                                u32RepeatedlyKey = 0;
                                return E_KEYPAD_UP;
                            }
                        }
                        u32CheckTime = u32CheckTime+1;
                        u32CheckKeyCode = u32KeyCode[u32Idx];
                    }
                }
                if(u32CheckTime<MAX_KEY_ARRY/2)
                {
                    u32LastKey = 0;
                    u32RepeatedlyKey = 0;
                    return E_KEYPAD_UP;
                }

            }
#endif
#if 0
            if(u32LastKey!=g_KeyCode)
            {
                u32LastKey = g_KeyCode;
                u32RepeatedlyKey = 0;
                return E_KEYPAD_UP;
            }
            else
            {
                u32RepeatedlyKey = u32RepeatedlyKey+1;
                if(u32RepeatedlyKey<3)
                    return E_KEYPAD_UP;
            }
#endif
            *pu32KeyCode = g_KeyCode;
            return Successful;
        }
        else /* Key not press */
        {
            *pu32KeyCode = 0;
            u32RepeatedlyKey = 0;
            u32LastKey = 0;
            adcSetOperationMode(E_ADC_NONE);
            return E_KEYPAD_UP;
        }
    }
    return E_ADC_BUSY;
#else
    if(adcGetOperationMode() == E_ADC_NONE)
    {
        adcSetOperationMode(E_ADC_KEY_WAIT);
        DrvADC_Channel(u32Channel);
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) | INT_KEY));  /* Write one clear */
        DrvADC_EnableInt(eADC_KEY);
        return Successful;
    }
    return E_ADC_BUSY;
#endif

}
/*
    The function should be called every 20ms.
*/
INT32 DrvADC_VoltageDetection(UINT32 u32Channel)
{
    if(adcGetOperationMode() == E_ADC_NONE)
    {
        adcSetOperationMode(E_ADC_VD);
        DrvADC_Channel(u32Channel);
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) | INT_NOR));  /* Write one clear */
        DrvADC_EnableInt(eADC_AIN);

        //outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | (XP_EN | IN_SEL));   /*  test only */
        //outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | (IN_SEL));   /*  test only */
        //outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | XP_EN ); /*  test only */

        outp32(REG_TP_CTL1, inp32(REG_TP_CTL1) | SW_GET);
        return Successful;
    }
    return E_ADC_BUSY;
}




/*-----------------------------------------------------------------------------------------------------------
    Function adc_read.
        Get touch panel position.
    mode: ADC_NONBLOCK or ADC_BLOCK.
    x: X position
    y: Y position

    return 0: No any position.  The (x, y) is unavilable.
             1: Panel was touched. The (x, y) is avilable.
-----------------------------------------------------------------------------------------------------------*/
#define SORT_FIFO 6
/* =============================== sorting =============================== */
static void swap(UINT16 *x,UINT16 *y)
{
    UINT16 temp;
    temp = *x;
    *x = *y;
    *y = temp;
}
static UINT16 choose_pivot(UINT16 i,UINT16 j )
{
    return((i+j) /2);
}
static void quicksort(UINT16 list[],int m,int n)
{
    int key,i,j,k;
    if( m < n)
    {
        k = choose_pivot(m,n);
        swap(&list[m],&list[k]);
        key = list[m];
        i = m+1;
        j = n;
        while(i <= j)
        {
            while((i <= n) && (list[i] <= key))
                i++;
            while((j >= m) && (list[j] > key))
                j--;
            if( i < j)
                swap(&list[i],&list[j]);
        }
        // swap two elements
        swap(&list[m],&list[j]);
        // recursively sort the lesser list
        quicksort(list,m,j-1);
        quicksort(list,j+1,n);
    }
}

#define QUEUE_SIZE  10
typedef struct tagQ
{
    UINT16 X;
    UINT16 Y;
} S_POS;
S_POS Queue[QUEUE_SIZE];
static INT16 front=0, rear=0;
static UINT16 rollback = 0;
static void pushQueue(UINT16 x, UINT16 y)
{
    Queue[front].X = x;
    Queue[front].Y = y;
    front = front+1;
    if(front >= QUEUE_SIZE)
    {
        front = 0;
        rollback = 1;
        rear = QUEUE_SIZE - 2;      /* Alway skip last 2 data */
    }
}
/*-----------------------------------------------------------------------------------------------------------
Parameter:
  x: X position
    y: Y position

    return 0: Invaliable data
             1: Valiable data
-----------------------------------------------------------------------------------------------------------*/
static INT32 popQueue(UINT16* x, UINT16* y)
{
    if(rollback==0)
    {
        if(front >= 5)
        {
            /* Skip first (5-3) position to avoid charge issue */
            rear = front-3; /* Not set to same as front to avoid the discharge issue */
        }
        else
            return 0;           /* Invaliable data */
    }
    *x = Queue[rear].X;
    *y = Queue[rear].Y;
    rear = rear+1;
    if(rear>=QUEUE_SIZE)
        rear = 0;
    return 1;   /* Valiable data */
}
/*-----------------------------------------------------------------------------------------------------------
Parameter:
  None

    return 0:                Pen up
         1:                Pen down
         E_ADC_BUSY:       ADC busy
-----------------------------------------------------------------------------------------------------------*/
INT32 IsPenDown(void)
{
    UINT32 u32OperationMode = adcGetOperationMode();
    if(u32OperationMode == E_ADC_NONE)
    {
        adcSetOperationMode(E_ADC_TP_GETXY);
        outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  & ~TSMODE);
        outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  | IN_SEL);
        outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(XP_EN | XM_EN | YP_EN)) | (YM_EN| PLLUP) );

        DrvADC_DisableInt(eADC_TOUCH);
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY)) | INT_TC);  /* Write one clear */
        //TouchDelay(5);
        //if((inp32(REG_TP_INTST)&INT_TC)==INT_TC)
        if(TouchDelayCheckPenDown(50) == 1)                 /* Due to PLLUP bit cause the INT_TC was set. */
        {
            //Pen down
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY)) | INT_TC);  /* Write one clear */
            adcSetOperationMode(E_ADC_NONE);
            return 1;       /* Pen down */
        }
        else
        {
            front=0,
            rear=0;
            rollback = 0;
            adcSetOperationMode(E_ADC_NONE);
            return 0;       /* Pen up */
        }
    }
    else
    {
        return E_ADC_BUSY;  /* ADC Busy */
    }
}

/*-----------------------------------------------------------------------------------------------------------
Parameter:
  mode:  Useless
  x:     X Position if pen down
  y:     Y Position if pen down

    return 0:                Pen up
         1:                Pen down
         E_ADC_BUSY:       ADC busy
-----------------------------------------------------------------------------------------------------------*/
static UINT16 au16XPos[SORT_FIFO];
static UINT16 au16YPos[SORT_FIFO];
//static UINT16 u16LastX=0, u16LastY =0;
INT32 adc_read(unsigned char mode, unsigned short int *x, unsigned short int *y)
{
    UINT32 i;

    UINT32 u32OperationMode = adcGetOperationMode();
    if(u32OperationMode == E_ADC_NONE)
    {
        adcSetOperationMode(E_ADC_TP_GETXY);
        DrvADC_DisableInt(eADC_TOUCH);

        outp32(REG_TP_CTL2, 0xFF);
        outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY)) | INT_TC);  /* Write one clear */


        DrvADC_DisableInt(eADC_AIN);    //DrvADC_EnableInt(eADC_AIN);
        /* Force ADC to get (X, Y) Position */
        for(i = 0; i<SORT_FIFO; i=i+1)
        {
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(SW_GET|GET_PRESSURE|GET_XY|GET_X|GET_Y)) | GET_XY);
            while( (inp32(REG_TP_INTST)&INT_NOR) == 0); //Until ADC done
            outp32(REG_TP_INTST, INT_NOR);      /* Clear INT */
            TouchDelay(5);
            au16XPos[i] = (inp32(REG_XY_DATA)>>16) & 0xFFF;
            au16YPos[i] = inp32(REG_XY_DATA) & 0xFFF;

            /* many improvement for fat finger issue*/
            outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  & ~TSMODE);
            outp32(REG_TP_CTL1, inp32(REG_TP_CTL1)  | IN_SEL);
            outp32(REG_TP_CTL1, (inp32(REG_TP_CTL1) & ~(XP_EN | XM_EN | YP_EN)) | (YM_EN| PLLUP) );
            DrvADC_DisableInt(eADC_TOUCH);
            outp32(REG_TP_INTST, (inp32(REG_TP_INTST) & ~(INT_NOR|INT_KEY)) | INT_TC);  /* Write one clear */
            TouchDelay(5);          /* Due to PLLUP bit cause the INT_TC was set. */
            if((inp32(REG_TP_INTST)&INT_TC)!=INT_TC)
            {
                adcSetOperationMode(E_ADC_NONE);
                return 0;  /* Return pen up */
            }
            else
            {

            }
            /* check pen up */
        }
        adcSetOperationMode(E_ADC_NONE);


        quicksort(au16XPos, 0, SORT_FIFO-1);
        quicksort(au16YPos, 0, SORT_FIFO-1);
        *x = (au16XPos[SORT_FIFO-2]+au16XPos[SORT_FIFO-3])/2;
        *y = (au16YPos[SORT_FIFO-2]+au16YPos[SORT_FIFO-3])/2;
#if 1 /* a little bit improvement */
        pushQueue(*x, *y);              /* push new data to queue for charge/discharge issue */
        if( popQueue(x, y) == 1 )       /* To pop old data for charge and discharge issue */
            return 1;
        else
            return 0;
#else
        return 1;
#endif
    }
    else
    {
        return E_ADC_BUSY;
    }
}
