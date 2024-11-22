#ifndef __My__ADC__H
#define __My__ADC__H
#include "FreeRTOS.h"	  // ARM.FreeRTOS::RTOS:Core
#include "task.h"		  // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"		  // ARM.FreeRTOS::RTOS:Core 
#include "queue.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "adc.h"
#define READCQueueLenght 1
typedef struct
{
    float air;
    float light;
} ADCx_Queue_Data;
void My_ADC_Init(void);
void ADC_GETDATA_Thread_Start(void *PThread, StackType_t *PThread_Stack, int LMotor_Stack);
QueueHandle_t ReADC_QueueStruct(void);

#endif //__Main_TASK_H
