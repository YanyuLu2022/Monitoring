#include "FreeRTOS.h"	  // ARM.FreeRTOS::RTOS:Core
#include "task.h"		  // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"		  // ARM.FreeRTOS::RTOS:Core
#include "cmsis_os.h"
#include "queue.h"
#include "adc.h"
#include "string.h"
#include "Main_Task.h"
#include "My_can.h"
#include "My_bluetooth.h"
#include "My_ADC.h"
#include "DHT11.h"

static uint8_t T = 0,H = 0;
static uint8_t lig = 0;
static float air,light;
static StackType_t Main_StackBuffer[256];	 // 主任务堆栈空间
static StackType_t Can_Tx_StackBuffer[256];	 // /Can发送任务堆栈空间
static StackType_t ADC_StackBuffer[256];	 // /ADC发送任务堆栈空间
static StackType_t DHT11_StackBuffer[256];	 // /DHT11发送任务堆栈空间

static StaticTask_t Main_Task;  // 主任务块
static StaticTask_t Can_TxTask;  //Can发送任务块
static StaticTask_t ADC_Task;  //ADC发送任务块
static StaticTask_t DHT11_Task;  //DHT11发送任务块

static CAN_FilterTypeDef can_Rx_default;//过滤器初始化结构体
static CAN_TxHeaderTypeDef can_Tx_default;//发送初始化结构体


static QueueHandle_t CAN_TxQueue;	   // CAN_Tx队列句柄
static QueueHandle_t ADC_Queue;	   // ADC队列句柄
static QueueHandle_t Uart_Queue;	   // 串口队列句柄
static QueueHandle_t DHT11_Queue;	   // DHT11队列句柄

static QueueHandle_t g_xQueueStrInput; // 队列集句柄

static CANTx_Queue_Data TxData;//发送句柄
void Main_Thread_Start(void * PThread,StackType_t * PThread_Stack,int LMotor_Stack);
void Main_Init_Start(void)
{

    Init_My_CAN();
	USART_Init();
	My_ADC_Init();
    My_DHT11_Init();
    CAN_DATA_Init(&can_Rx_default,&can_Tx_default);//默认值初始化
    can_Tx_default.DLC = 3;
    can_Tx_default.StdId = 0x123;
	HAL_ADC_Start(&hadc1);
    Can_Write_Thread_Start(&Can_TxTask, Can_Tx_StackBuffer, sizeof(Can_Tx_StackBuffer)/sizeof(Can_Tx_StackBuffer[0]),&can_Tx_default);
    ADC_GETDATA_Thread_Start(&ADC_Task,ADC_StackBuffer,sizeof(ADC_StackBuffer)/sizeof(ADC_StackBuffer[0]));   
    DHT11_GETDATA_Thread_Start(&DHT11_Task,DHT11_StackBuffer,sizeof(DHT11_StackBuffer)/sizeof(DHT11_StackBuffer[0]));

    Main_Thread_Start(&Main_Task,Main_StackBuffer,sizeof(Main_StackBuffer)/sizeof(Main_StackBuffer[0]));
}

void Read_ADC_Queue(void)
{
    // 获取ADC数据并入队
    ADCx_Queue_Data adc_data;
    xQueueReceive(ADC_Queue,&adc_data,0);
    air = adc_data.air;
    light = adc_data.light;

}
void Read_DHT11_Queue(void)
{
    // 获取ADC数据并入队

    DHT11_Queue_Data dht11_data;
    xQueueReceive(DHT11_Queue,&dht11_data,0);
    T = dht11_data.temp;
    H = dht11_data.humidity;

}


void OutUsartPrintf(void)
{
    UsartPrintf(&huart1,1000,"T: %d,D: %d,air: %.2f,light: %.2f \r\n",T,H,air,light);

}
void Main_Thread(void *argument)
{

    	// 创建队列集,队列集的大小为所有队列最大值相加
	g_xQueueStrInput = xQueueCreateSet(UartQueueLenght + READCQueueLenght + REDHT11QueueLenght);
    
    Uart_Queue = Return_P_UartStruct();
    CAN_TxQueue = ReCanTxQueueStruct();
    ADC_Queue = ReADC_QueueStruct();
    DHT11_Queue = ReDHT11_QueueStruct();
	xQueueAddToSet(Uart_Queue, g_xQueueStrInput);
	xQueueAddToSet(ADC_Queue, g_xQueueStrInput);    
 	xQueueAddToSet(DHT11_Queue, g_xQueueStrInput);       
    while (1)
    {

		QueueSetMemberHandle_t IntupQueue;
		// 读队列集得到句柄
		

#if 0
       TxData.directive = 0x01;
       TxData.MOD = 0x03;
       xQueueSend(CAN_TxQueue, &TxData, 0);//发送CAN数据
#endif

		IntupQueue = xQueueSelectFromSet(g_xQueueStrInput, pdPASS);
		if (IntupQueue == ADC_Queue)
		{
            Read_ADC_Queue();
		} 	
		if (IntupQueue == DHT11_Queue)
		{
            Read_DHT11_Queue();
		} 
        OutUsartPrintf();
        vTaskDelay(200);
    }
    

}


void Main_Thread_Start(void * PThread,StackType_t * PThread_Stack,int LMotor_Stack)
{
    xTaskCreateStatic(Main_Thread,
								  "main_mod",
								  LMotor_Stack,
								  NULL,
								  (osPriority_t)osPriorityNormal,
								  PThread_Stack,
								  PThread);

}

