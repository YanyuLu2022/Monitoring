#include "FreeRTOS.h"     // ARM.FreeRTOS::RTOS:Core
#include "task.h"         // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h" // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"       // ARM.FreeRTOS::RTOS:Core
#include "cmsis_os.h"
#include "queue.h"
#include "adc.h"
#include "string.h"
#include "Main_Task.h"
#include "My_can.h"
#include "My_ADC.h"
#include "DHT11.h"
#include "MY_Voice.h"
#include "My_bluetooth.h"
static uint8_t T = 0, H = 0;
//  static uint8_t lig = 0;
static float air, light;
static uint8_t led_state = 0;
static StackType_t Main_StackBuffer[256];   // 主任务堆栈空间
static StackType_t Can_Tx_StackBuffer[256]; // /Can发送任务堆栈空间
static StackType_t ADC_StackBuffer[256];    // /ADC发送任务堆栈空间
static StackType_t DHT11_StackBuffer[256];  // /DHT11发送任务堆栈空间

static StaticTask_t Main_Task;  // 主任务块
static StaticTask_t Can_TxTask; // Can发送任务块
static StaticTask_t ADC_Task;   // ADC任务块
static StaticTask_t DHT11_Task; // DHT11任务块
static StaticTask_t Mp3_Task;   // Mp3任务快
static StaticTask_t SU10T_Task; // SU10T任务快
static StaticTask_t BLUERX_Task;   // BLUERX任务快
static StaticTask_t BLUETX_Task;   //BLUETX任务快

static CAN_FilterTypeDef can_Rx_default;   // 过滤器初始化结构体
static CAN_TxHeaderTypeDef can_Tx_default; // 发送初始化结构体

static QueueHandle_t CAN_TxQueue; // CAN_Tx队列句柄
static QueueHandle_t ADC_Queue;   // ADC队列句柄
static QueueHandle_t MP3_Queue;   // 队列句柄
static QueueHandle_t SU10T_Queue; // SU10T队列句柄
static QueueHandle_t DHT11_Queue; // DHT11队列句柄
static QueueHandle_t BlueTx_Queue; // BlueTx队列句柄
static QueueHandle_t BlueRx_Queue; // BlueRx队列句柄

static QueueHandle_t g_xQueueStrInput; // 队列集句柄

static CANTx_Queue_Data TxData; // 发送句柄
/**
 * mode
0：不变
1：蓝牙模式
2：语音模式
3：自动驾驶模式
4: 静止模式
5：手柄模式
 */
static uint8_t g_mode = 0;
void Len_Init(void);
void Main_Thread_Start(void *PThread, StackType_t *PThread_Stack, int LMotor_Stack);
void Main_Init_Start(void)
{

    Init_My_CAN();
    My_ADC_Init();
    My_DHT11_Init();
    Voice_Init();
    Blue_Init();
    Len_Init();
    CAN_DATA_Init(&can_Rx_default, &can_Tx_default); // CAN赋默认值
    can_Tx_default.DLC = 3;
    can_Tx_default.StdId = 0x123;
    HAL_ADC_Start(&hadc1);
    Can_Write_Thread_Start(&Can_TxTask, Can_Tx_StackBuffer, sizeof(Can_Tx_StackBuffer) / sizeof(Can_Tx_StackBuffer[0]), &can_Tx_default);
    ADC_GETDATA_Thread_Start(&ADC_Task, ADC_StackBuffer, sizeof(ADC_StackBuffer) / sizeof(ADC_StackBuffer[0]));
    DHT11_GETDATA_Thread_Start(&DHT11_Task, DHT11_StackBuffer, sizeof(DHT11_StackBuffer) / sizeof(DHT11_StackBuffer[0]));
    Main_Thread_Start(&Main_Task, Main_StackBuffer, sizeof(Main_StackBuffer) / sizeof(Main_StackBuffer[0]));
    SU10T_Thread_Start(&SU10T_Task, NULL);
    MP3_Thread_Start(&Mp3_Task, NULL);
    BlueRx_Thread_Start(&BLUERX_Task, NULL);
    BlueTx_Thread_Start(&BLUETX_Task, NULL);    
}
void Write_Blue(void)
{
    BlueTx_Struct BlueTx_data;
    BlueTx_data.T = T;
    BlueTx_data.H = H;
    BlueTx_data.air = air;
    BlueTx_data.light = light;
    BlueTx_data.car_modl = g_mode;
    BlueTx_data.led = led_state;
    xQueueSend(BlueTx_Queue, &BlueTx_data, 0);
}

void Len_Init(void) 
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
}


/**
 * mode
0：不变
1：蓝牙模式
2：语音模式
3：自动驾驶模式
4: 静止模式
5：手柄模式
 */
void Write_CAN_Car(uint8_t directive)
{

    TxData.directive = directive;
    switch (g_mode)
    {
    case 1:
        TxData.MOD = 1;
        break;
    case 2:
        TxData.MOD = 2;       
        break;
     case 3:
        TxData.MOD = 3;       
        break;
    case 4:
        TxData.MOD = 0;        
        break;
    case 5:
        TxData.MOD = 2;        
        break;                           
    default:

        break;
    }
	Write_Blue();
    xQueueSend(CAN_TxQueue, &TxData, 0); // 发送CAN数据

}

void Set_Led(uint8_t led) {
    if(led == 1)
    {
        led_state = 1;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    }
    else
    {
        led_state = 0;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);


    }
    Write_Blue();
}
void Read_ADC_Queue(void)
{
    // 获取ADC数据并入队
    ADCx_Queue_Data adc_data;
    
    xQueueReceive(ADC_Queue, &adc_data, 0);
    air = adc_data.air;
    light = adc_data.light;
	Write_Blue();
    
}
void Read_DHT11_Queue(void)
{
    // 获取ADC数据并入队

    DHT11_Queue_Data dht11_data;
    xQueueReceive(DHT11_Queue, &dht11_data, 0);
    T = dht11_data.temp;
    H = dht11_data.humidity;
	Write_Blue();
}
void Read_SU10T_Queue(void)
{
    SU10T_Struct SU10T_data;
    MP3_Struct Mp3_data = {0};
    xQueueReceive(SU10T_Queue, &SU10T_data, 0);
    switch (SU10T_data.instruct)
    {
    case 0x01:
        Mp3_data.voice = 4;
        // 发送MP3数据
        break;
    case 0x02:
        Mp3_data.voice = 1;
        Mp3_data.music = 1;
        break;
    case 0x03:
        Mp3_data.voice = 1;
        Mp3_data.music = 4;
        break;
    case 0x04:
        Mp3_data.voice = 1;
        Mp3_data.volume = 2;

        break;
    case 0x05:
        Mp3_data.voice = 1;
        Mp3_data.volume = 1;
        break;
    case 0x06:
        Mp3_data.voice = 1;
        Mp3_data.music = 3;
        break;
    case 0x07:
        Mp3_data.voice = 1;
        Mp3_data.music = 2;
        break;
    case 0x08:
        Mp3_data.voice = 1;
        g_mode = 2;
        Write_CAN_Car(2);
        // 前
        break;
    case 0x09:
        Mp3_data.voice = 1;
        g_mode = 2;
        Write_CAN_Car(3);
        // 后
        break;
    case 0x0A:
        Mp3_data.voice = 1;
        g_mode = 2;
        Write_CAN_Car(4);
        // 左
        break;
    case 0x0B:
        Mp3_data.voice = 1;
        g_mode = 2;
        Write_CAN_Car(5);
        // 右
        break;
    case 0x0C:
        g_mode = 2;
        Write_CAN_Car(8);
        // 加速
        break;
    case 0x0D:
        g_mode = 2;
        Write_CAN_Car(9);
        // 减速
        break;
    case 0x0E:
        g_mode = 2;
        Write_CAN_Car(1);
        // 停车
        break;
    case 0x0F:
        Mp3_data.voice = 1;
        g_mode = 2;
        Write_CAN_Car(6);
        // 左转
        break;
    case 0xA1:
        Mp3_data.voice = 1;
        g_mode = 2;
        Write_CAN_Car(7);
        
        // 右转
        break;
    case 0xA2:
        Mp3_data.voice = 1;
        // 切换模式
        ++g_mode;
        if(g_mode > 5)g_mode = 1;
        Write_CAN_Car(0);
        break;
    case 0xA3:
        Mp3_data.voice = 1;
        Set_Led(1);
        // 开灯
        break;
    case 0xA4:
        Mp3_data.voice = 1;
        Set_Led(0);
        // 关灯
        break;
    case 0xA5:
        // 你干嘛
        Mp3_data.voice = 3;
        break;

    default:
        break;
    }
    xQueueSend(MP3_Queue, &Mp3_data, 0);
}

void Read_BlueRx_Queue(void)
{
    BlueRx_Struct BlueRx_Data = {0};
    MP3_Struct Mp3_data = {0};

    xQueueReceive(BlueRx_Queue, &BlueRx_Data, 0);
    uint8_t Directive = 0;
    switch ( BlueRx_Data.directive)
    {
    case 0x01:
        Directive = 1;
        /* code */
        break;
     case 0x02:
        Directive = 2;
        /* code */
        break;
     case 0x03:
        Directive = 3;   
        /* code */
        break;
     case 0x04:
        Directive = 4;   
        /* code */
        break;
     case 0x05:
        Directive = 5;   
        /* code */
        break;
     case 0x06:
        Directive = 6;    
        /* code */
        break;
     case 0x07:
        Directive = 7;      
        /* code */
        break;
     case 0x08:
        Directive = 8;     
        /* code */
        break;
     case 0x09:
        Directive = 1;      
        /* code */
        break;
    default:
        break;
    }
   switch (BlueRx_Data.mod)
   {
   case 0x01:
    g_mode = 1;
    /* code */
    break;
   case 0x02:
   g_mode = 2;
    /* code */
    break;
   case 0x03:
   g_mode = 3;
    /* code */
    break;
   case 0x04:
   g_mode = 0;
    /* code */
    break;           
   default:
    break;
   }
   
   if(BlueRx_Data.directive != 0 && BlueRx_Data.mod != 0) Write_CAN_Car(Directive);
   
   switch (BlueRx_Data.music)
   {
   case 0x01 :
    Mp3_data.music = 1;
    break;
    case 0x02:
    Mp3_data.music = 2;
    /* code */
    break;
    case 0x03:
    Mp3_data.music = 3;

    /* code */
    break;
    case 0x04:
    Mp3_data.music = 4;
    /* code */
    break;
    case 0x05:
    Mp3_data.volume = 1;
    /* code */
    break;
    case 0x06:
    Mp3_data.volume = 2;

    /* code */
    break;
    case 0x07:
    Mp3_data.voice  = 2;    
    /* code */
    break;
    case 0x08:
    Mp3_data.voice  = 3;    

    /* code */
    break;
   default:
    break;
   }
    xQueueSend(MP3_Queue, &Mp3_data, 0);  
   
    if(BlueRx_Data.led == 0x01)
    {
        //开灯
        Set_Led(1);

    }else if(BlueRx_Data.led == 0x02)
    {
        //关灯
        Set_Led(0);

    }

   
    

}

void Main_Thread(void *argument)
{

    // 创建队列集,队列集的大小为所有队列最大值相加
    g_xQueueStrInput = xQueueCreateSet(READCQueueLenght + REDHT11QueueLenght + SU10TQueueLenght + BlueRxQueueLenght);


    CAN_TxQueue = ReCanTxQueueStruct();
    ADC_Queue = ReADC_QueueStruct();
    DHT11_Queue = ReDHT11_QueueStruct();
    MP3_Queue = ReMP3_QueueStruct();
    SU10T_Queue = ReSU10T_QueueStruct();
    BlueTx_Queue = ReBlueTx_QueueStruct(); // BlueTx队列句柄
    BlueRx_Queue = ReBlueRx_QueueStruct(); // BlueRx队列句柄
    xQueueAddToSet(ADC_Queue, g_xQueueStrInput);
    xQueueAddToSet(DHT11_Queue, g_xQueueStrInput);
    xQueueAddToSet(SU10T_Queue, g_xQueueStrInput);
    xQueueAddToSet(BlueRx_Queue, g_xQueueStrInput);
    while (1)
    {

        QueueSetMemberHandle_t IntupQueue;
        // 读队列集得到句柄
        IntupQueue = xQueueSelectFromSet(g_xQueueStrInput, portMAX_DELAY);
        if (IntupQueue == ADC_Queue)
        {
            Read_ADC_Queue();
        }else
        if (IntupQueue == DHT11_Queue)
        {
            Read_DHT11_Queue();
        }else
        if (IntupQueue == SU10T_Queue)
        {
            Read_SU10T_Queue();
        }else
        if (IntupQueue == BlueRx_Queue)
        {
            Read_BlueRx_Queue();
        }
    }
}

void Main_Thread_Start(void *PThread, StackType_t *PThread_Stack, int LMotor_Stack)
{
    xTaskCreateStatic(Main_Thread,
                      "main_mod",
                      LMotor_Stack,
                      NULL,
                      (osPriority_t)osPriorityNormal,
                      PThread_Stack,
                      PThread);
}
