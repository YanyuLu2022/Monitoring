#ifndef __Main_TASK_H
#define __Main_TASK_H
#include "stm32f1xx_hal.h"
#include "Freertos.h"
#include "gpio.h"
#include "FreeRTOS.h"  
#include "queue.h"


void Main_Init_Start(void);
void Main_Thread_Start(void * PThread,StackType_t * PThread_Stack,int LMotor_Stack);


#endif //__Main_TASK_H
