#include "stdio.h"
#include "string.h"
#include <stdarg.h>
#include <stdlib.h>

#include "My_Voice.h"
#include "My_Usart.h"

#define MP3_Thread_lenght 128   // 任务堆栈大小
#define SU10T_Thread_lenght 128 // 任务堆栈大小

static StackType_t MP3_Thread_Buffer[MP3_Thread_lenght];     // 任务存储数组
static StackType_t SU10T_Thread_Buffer[SU10T_Thread_lenght]; // 任务存储数组

static QueueHandle_t MP3_Queue = NULL;   // 队列句柄
static QueueHandle_t SU10T_Queue = NULL; // 队列句柄

static int8_t vol = 0x08;  // 声音
static uint8_t mus = 0x01; // 音乐
static uint8_t vce = 0x01; // 提示

QueueHandle_t ReMP3_QueueStruct(void) // 返回队列句柄
{
    return MP3_Queue;
}
QueueHandle_t ReSU10T_QueueStruct(void) // 返回队列句柄
{
    return SU10T_Queue;
}

void Voice_Init(void)
{
    MP3_Queue = xQueueCreate(MP3QueueLenght, sizeof(MP3_Struct)); // 创建队列
    SU10T_Queue = xQueueCreate(SU10TQueueLenght, sizeof(SU10T_Struct));
	My_Uart1_Init();

}
uint8_t Voice_Get(uint8_t Svce)
{
    if (Svce != 0)
        vce = Svce;
    return vce;
}

uint8_t Music_Get(uint8_t Smus)
{
    if (Smus == 1)
    {
        mus = 0X01;
        return mus;
    }
    else if (Smus == 2)
    {
        mus += 1;
        if (mus > 0x07)
        {
            mus = 0x01;
        }
        return mus;
    }
    else if (Smus == 3)
    {
        mus -= 1;
        if (mus < 0x01)
        {
            mus = 0x07;
        }
        return mus;
        ;
    }
    return mus;
}

uint8_t Volume_Get(uint8_t Svol)
{
    if (Svol == 1)
    {
        vol -= 2;
        if (vol < 0)
        {
            vol = 0x00;
        }
    }
    else if (Svol == 2)
    {

        vol += 2;
        if (vol > 0x1E)
        {
            vol = 0x1E;
        }
    }
    return vol;
}
void SU10T_Thread(void *argument)
{
    static Uart1_Struct Uart1Rx_Buffer; // 接收缓冲区
    int rx_n = 0;
    QueueHandle_t Uart_queue = Return1_P_UartStruct();
    while (1)
    {

        xQueueReceive(Uart_queue, &Uart1Rx_Buffer, portMAX_DELAY);

        static SU10T_Struct SU10T_Data;
        for (rx_n = 0; rx_n < Uart1Rx_Buffer.num; rx_n++)
        {
            if (Uart1Rx_Buffer.rx_str[rx_n] == 0x7E)
            {
                if ((rx_n + 5) > Uart1Rx_Buffer.num)
                    break;

                if (Uart1Rx_Buffer.rx_str[rx_n + 0] == 0x7E &&
                    Uart1Rx_Buffer.rx_str[rx_n + 1] == 0XFF &&
                    Uart1Rx_Buffer.rx_str[rx_n + 2] == 0X06 &&
                    Uart1Rx_Buffer.rx_str[rx_n + 3] == Uart1Rx_Buffer.rx_str[rx_n + 4] &&
                    Uart1Rx_Buffer.rx_str[rx_n + 5] == 0XEF)
                {

                    SU10T_Data.instruct = Uart1Rx_Buffer.rx_str[rx_n + 3];

                    xQueueSend(SU10T_Queue, &SU10T_Data, 0);
                }
            }
        }
    }
}
void MP3_Thread(void *argument)
{

    static MP3_Struct MP3_Data = {0};
    static char tx_str[8] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x08, 0xEF}; // 默认音量

    vTaskDelay(200);
    Usart_TX_Data(1, 1000, tx_str, 8); // 调节音量
    while (1)
    {

        xQueueReceive(MP3_Queue, &MP3_Data, portMAX_DELAY); // 队列接收数据
        if (MP3_Data.volume != 0)
        {
            tx_str[2] = 0x06;
            tx_str[3] = 0x06;
            tx_str[4] = 0x00;
            tx_str[5] = 0x00;
            tx_str[6] = Volume_Get(MP3_Data.volume);
            Usart_TX_Data(1, 1000, tx_str, 8); // 队列发送数据
            vTaskDelay(10);                    // 任务延时
        }

        if (MP3_Data.voice != 0)
        {
            tx_str[2] = 0x06;
            tx_str[3] = 0x0F;
            tx_str[4] = 0x00;
            tx_str[5] = 0x02;
            tx_str[6] = Voice_Get(MP3_Data.voice);
            Usart_TX_Data(1, 1000, tx_str, 8); // 队列发送数据
            vTaskDelay(2500);                    // 任务延时
 
        }
        if (MP3_Data.music != 0)
        {
            if (MP3_Data.music == 4)
            {
                tx_str[2] = 0x06;
                tx_str[3] = 0x16;
                tx_str[4] = 0x00;
                tx_str[5] = 0x00;
                tx_str[6] = 0x00;
            }
            else
            {
                tx_str[2] = 0x06;
                tx_str[3] = 0x12;
                tx_str[4] = 0x00;
                tx_str[5] = 0x00;
                tx_str[6] = Music_Get(MP3_Data.music);
            }
            Usart_TX_Data(1, 1000, tx_str, 8); // 队列发送数据
            vTaskDelay(10);                    // 任务延时
        }
        //xQueueReceive(MP3_Queue, &MP3_Data, 0);             

    }

} // 任务

void MP3_Thread_Start(StaticTask_t *PThread, void *Parameters)
{
    uint32_t MP3_Stack_L = sizeof(MP3_Thread_Buffer) / sizeof(MP3_Thread_Buffer[0]);
    xTaskCreateStatic(MP3_Thread,
                      "MP3",
                      MP3_Stack_L,
                      Parameters,
                      (osPriority_t)osPriorityNormal,
                      MP3_Thread_Buffer,
                      PThread);
}

void SU10T_Thread_Start(StaticTask_t *PThread, void *Parameters)
{
    uint32_t SU10T_Stack_L = sizeof(SU10T_Thread_Buffer)/ sizeof(SU10T_Thread_Buffer[0]);
    xTaskCreateStatic(SU10T_Thread,
                      "SU10T",
                      SU10T_Stack_L,
                      Parameters,
                      (osPriority_t)osPriorityNormal,
                      SU10T_Thread_Buffer,
                      PThread);
}
