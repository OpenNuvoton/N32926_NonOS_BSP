/*
 * task_state.c - get all of the task information
 */

/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Nuvoton includes. */
#include "wblib.h"

#define MAX_TASK_NUM		64
TaskStatus_t pxTaskStatusArray[MAX_TASK_NUM];

/*-----------------------------------------------------------*/
#if (configGENERATE_RUN_TIME_STATS == 1)
void vConfigureTimerForRunTimeStats( void )
{
	uint32_t prescale, reg_value;

	// enable the Timer 2 clock
	outp32(REG_APBCLK, inp32(REG_APBCLK) | TMR2_CKE);
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) | TMR2RST);
	outp32(REG_APBIPRST, inp32(REG_APBIPRST) & ~TMR2RST);


	// Just set a any value, but must not be 0 or 1
	outpw(REG_TICR2, 0xFF000000);

	// The tick interval is 1/(12M/(239+1)) = 20 us
	prescale = 239;

	// Reg value = CEN | uninterrupted mode | TDR_EN | PRESCALE
	reg_value = BIT30 | (0x3 << 27) | BIT16 | prescale;

	outpw(REG_TCSR2, reg_value);
}

uint32_t vGetRunTimeCounterValue( void )
{
	return inpw(REG_TDR2);
}
#endif // (configGENERATE_RUN_TIME_STATS == 1)
/*-----------------------------------------------------------*/

// get all task information
void vGetAllTaskState(void)
{
	const char task_state[] = { 'r', 'R', 'B', 'S', 'D'};
	volatile UBaseType_t uxArraySize, i;
	uint32_t ulTotalRunTime, ulCountOffset, ulStatsAsPercentage;

	// get total task number
	uxArraySize = uxTaskGetNumberOfTasks();
	if (uxArraySize > MAX_TASK_NUM) {
		sysprintf("Task number of task exceeds the limit !!\n");
		return;
	}

	// get all task state in system
	uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
	ulCountOffset = ulTotalRunTime / 2;

	#if (configGENERATE_RUN_TIME_STATS == 1)

	vTaskSuspendAll();
	sysprintf("\n========================================================================\n");
	sysprintf(" TaskName	ID	State	Priority	CPU%%		Stack\n");

	// confirm the run time is not zero
	if (ulTotalRunTime > 0) {
		for (i = 0; i < uxArraySize; i++) {
			ulStatsAsPercentage = ((uint64_t)(pxTaskStatusArray[i].ulRunTimeCounter)*100 + ulCountOffset) / ulTotalRunTime;
			sysprintf("%-16s%2d%11c%11d%11d%%%17d\n", 
							pxTaskStatusArray[i].pcTaskName, 
							pxTaskStatusArray[i].xTaskNumber, 
							task_state[pxTaskStatusArray[i].eCurrentState], 
							pxTaskStatusArray[i].uxCurrentPriority, 
							ulStatsAsPercentage, 
							pxTaskStatusArray[i].usStackHighWaterMark);
		}
	}  
	sysprintf("State:  r-Running  R-Ready  B-Blocked  S-Suspended  D-Deleted\n");
	sysprintf("========================================================================\n");
	//( void ) xTaskResumeAll();		// comment it to keep the result in screen

	#endif //#if (configGENERATE_RUN_TIME_STATS==1)
}
