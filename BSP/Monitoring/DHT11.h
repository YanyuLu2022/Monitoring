#ifndef __DHT11__H
#define __DHT11__H
#include "FreeRTOS.h"	  // ARM.FreeRTOS::RTOS:Core
#include "task.h"		  // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"		  // ARM.FreeRTOS::RTOS:Core 
#include "queue.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "cmsis_os.h"
#include <string.h>
#define REDHT11QueueLenght 1
typedef struct
{
    uint8_t temp;
    uint8_t humidity;
} DHT11_Queue_Data;
 
//IO方向设置

 


 
void My_DHT11_Init(void);
void DHT11_GETDATA_Thread_Start(void *PThread, StackType_t *PThread_Stack, int LMotor_Stack);
QueueHandle_t ReDHT11_QueueStruct(void);

#endif //__Main_TASK_H
