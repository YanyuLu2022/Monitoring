#include "stdio.h"
#include "string.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"     // ARM.FreeRTOS::RTOS:Core
#include "task.h"         // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"       // ARM.FreeRTOS::RTOS:Core
#include <stdarg.h>
#include <stdlib.h>

#include "MY_bluetooth.h"
#include "My_Usart.h"

#define BlueTx_Thread_lenght 128 // 任务堆栈大小
#define BlueRx_Thread_lenght 128 // 任务堆栈大小

static StackType_t BlueTx_Thread_Buffer[BlueTx_Thread_lenght]; // 任务存储数组
static StackType_t BlueRx_Thread_Buffer[BlueRx_Thread_lenght]; // 任务存储数组

static QueueHandle_t BlueTx_Queue = NULL; // 队列句柄
static QueueHandle_t BlueRx_Queue = NULL; // 队列句柄
QueueHandle_t ReBlueTx_QueueStruct(void)  // 返回队列句柄
{
    return BlueTx_Queue;
}
QueueHandle_t ReBlueRx_QueueStruct(void) // 返回队列句柄
{
    return BlueRx_Queue;
}

void Blue_Init(void)
{
    BlueRx_Queue = xQueueCreate(BlueRxQueueLenght, sizeof(BlueRx_Struct)); // 创建队列
    BlueTx_Queue = xQueueCreate(BlueTxQueueLenght, sizeof(BlueTx_Struct));
    My_Uart2_Init();
}
void BlueRx_Thread(void *argument)
{
    static Uart2_Struct Uart2Rx_Buffer; // 接收缓冲区
    int rx_n = 0;
    QueueHandle_t Uart_queue = Return2_P_UartStruct();
    BlueRx_Struct BlueRx_Data = {0};
    while (1)
    {
        xQueueReceive(Uart_queue, &Uart2Rx_Buffer, portMAX_DELAY);
        for (rx_n = 0; rx_n < Uart2Rx_Buffer.num; rx_n++)
        {

            if (Uart2Rx_Buffer.rx_str[rx_n] == 0x7E)
            {
                if ((rx_n + 5) > Uart2Rx_Buffer.num)
                    break;
                if (Uart2Rx_Buffer.rx_str[rx_n + 5] != 0XEF)
                    break;
                BlueRx_Data.directive = Uart2Rx_Buffer.rx_str[rx_n + 1];
                BlueRx_Data.mod = Uart2Rx_Buffer.rx_str[rx_n + 2];
                BlueRx_Data.music = Uart2Rx_Buffer.rx_str[rx_n + 3];
                BlueRx_Data.led = Uart2Rx_Buffer.rx_str[rx_n + 4];
                xQueueSend(BlueRx_Queue, &BlueRx_Data, 0);
            }
        }

        /* code */
    }
}
void BlueTx_Thread(void *argument)
{

    static BlueTx_Struct BlueTx_Data = {0};

    while (1)
    {
        xQueueReceive(BlueTx_Queue, &BlueTx_Data, portMAX_DELAY);
        UsartPrintf(2,500, "+{T:%d,H:%d,light:%2.2f,air:%2.2f,led:%d,car_modl:%d}+",
        BlueTx_Data.T,BlueTx_Data.H,BlueTx_Data.light,BlueTx_Data.air,BlueTx_Data.led,BlueTx_Data.car_modl);
        vTaskDelay(200);
        
    }
}

void BlueTx_Thread_Start(StaticTask_t *PThread, void *Parameters)
{
    uint32_t BlueTx_Stack_L = sizeof(BlueTx_Thread_Buffer) / sizeof(BlueTx_Thread_Buffer[0]);
    xTaskCreateStatic(BlueTx_Thread,
                      "BlueTx",
                      BlueTx_Stack_L,
                      Parameters,
                      (osPriority_t)osPriorityNormal,
                      BlueTx_Thread_Buffer,
                      PThread);
}

void BlueRx_Thread_Start(StaticTask_t *PThread, void *Parameters)
{
    uint32_t BlueRx_Stack_L = sizeof(BlueRx_Thread_Buffer) / sizeof(BlueRx_Thread_Buffer[0]);
    xTaskCreateStatic(BlueRx_Thread,
                      "BlueRx",
                      BlueRx_Stack_L,
                      Parameters,
                      (osPriority_t)osPriorityNormal,
                      BlueRx_Thread_Buffer,
                      PThread);
}
