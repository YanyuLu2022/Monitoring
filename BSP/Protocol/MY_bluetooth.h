#ifndef MY_bluetooth_H
#define MY_bluetooth_H
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "FreeRTOS.h"  
#include "queue.h"
#include "usart.h"

#define BlueTxQueueLenght 5
#define BlueRxQueueLenght 5

typedef struct
{
    float air;//空气质量
	float light;//亮度
	uint8_t H;//湿度
	uint8_t T;//温度
	uint8_t led;//LED灯状态
	uint8_t car_modl;	
	
}BlueTx_Struct;//队列结构体


/*

MOD:切换模式
0: 不变
1：蓝牙模式
2：CAN模式
3：自动驾驶模式
4: 静止模式
directive:控制移动方向
0：不变
1：停
2：上
3：下
4：左
5：右
6：左转
7：右转
8：加速
9：减速
led
0：不变
1：开灯
2：关灯
music：
0：不变
1：播放音乐
2：上一首
3：下一首
4:关闭音乐
5:小声
6:大声
7：警报
8：你干嘛
*/
typedef struct
{
    uint8_t directive;//控制
	uint8_t mod;//模式
	uint8_t music;//音乐	
	uint8_t led;//led灯	
}BlueRx_Struct;//队列结构体

void Blue_Init(void);
QueueHandle_t ReBlueTx_QueueStruct(void);  // 返回队列句柄
QueueHandle_t ReBlueRx_QueueStruct(void); // 返回队列句柄
void BlueTx_Thread_Start(StaticTask_t *PThread, void *Parameters);
void BlueRx_Thread_Start(StaticTask_t *PThread, void *Parameters);



#endif // MY_bluetooth_H

