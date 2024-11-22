#include "DHT11.h"

#define DHT11_DQ_IN (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
void delay_us(uint32_t nus)
{
  uint32_t ticks;
  uint32_t tcnt = 0, told, tnow;
  uint32_t reload = SysTick->LOAD; // 重装载值

  ticks = nus * 72;    // 需要计的节拍数
  told = SysTick->VAL; // 刚进入while循环时计数器的值

  while (1)
  {
    tnow = SysTick->VAL;
    if (tnow != told)
    {
      if (tnow < told)
        tcnt += told - tnow;
      else
        tcnt += reload - (tnow - told);

      told = tnow; // 下次进入while循环时，当前VAL的值作为told

      if (tcnt >= ticks) // 已计的数超过/等于需要计的数时，退出循环
        break;
    }
  }
}
void DHT11_IO_IN(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void DHT11_IO_OUT_High(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}
void DHT11_IO_OUT_Low(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); 
}
// 复位DHT11
void DHT11_Rst(void)
{
  DHT11_IO_OUT_Low();  // 拉低DQ
  vTaskDelay(20);      // 拉低至少18ms
  DHT11_IO_OUT_High(); // 拉高20~40us
  delay_us(30);        // 主机拉高20~40us
}
// 等待DHT11的回应
// 返回1:未检测到DHT11的存在
// 返回0:存在
uint8_t DHT11_Check(void)
{
  uint8_t retry = 0;
  DHT11_IO_IN();                     // SET INPUT
  while (DHT11_DQ_IN && retry < 100) // DHT11会拉低40~80us
  {
    retry++;
    delay_us(1);
  };
  if (retry >= 100)
    return 1;
  else
    retry = 0;
  while (!DHT11_DQ_IN && retry < 100) // DHT11拉低后会再次拉高40~80us
  {
    retry++;
    delay_us(1);
  };
  if (retry >= 100)
    return 1;
  return 0;
}
// 从DHT11读取一个位
// 返回值：1/0
uint8_t DHT11_Read_Bit(void)
{
  uint8_t retry = 0;
  while (DHT11_DQ_IN && retry < 100) // 等待变为低电平
  {
    retry++;
    delay_us(1);
  }
  retry = 0;
  while (!DHT11_DQ_IN && retry < 100) // 等待变高电平
  {
    retry++;
    delay_us(1);
  }
  delay_us(40); // 等待40us
  if (DHT11_DQ_IN)
    return 1;
  else
    return 0;
}
// 从DHT11读取一个字节
// 返回值：读到的数据
uint8_t DHT11_Read_Byte(void)
{
  uint8_t i, dat;
  dat = 0;
  for (i = 0; i < 8; i++)
  {
    dat <<= 1;
    dat |= DHT11_Read_Bit();
  }
  return dat;
}
// 从DHT11读取一次数据
// temp:温度值(范围:0~50°)
// humi:湿度值(范围:20%~90%)
// 返回值：0,正常;1,读取失败
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi)
{
  uint8_t buf[5];
  uint8_t i;
  DHT11_Rst();
  if (DHT11_Check() == 0)
  {
    for (i = 0; i < 5; i++) // 读取40位数据
    {
      buf[i] = DHT11_Read_Byte();
    }
    if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
    {
      *humi = buf[0];
      *temp = buf[2];     
    }
  }
  else
    return 1;
  return 0;
}
// 队列句柄
static QueueHandle_t REDHT11_Queue;

uint16_t Read_DHT11Data[2] = {0};
void My_DHT11_Init(void)
{

  REDHT11_Queue = xQueueCreate(REDHT11QueueLenght, sizeof(DHT11_Queue_Data));
}
QueueHandle_t ReDHT11_QueueStruct(void)
{
  return REDHT11_Queue;
}
void DHT11_GETDATA_Thread(void *argument)
{
  DHT11_Queue_Data Get_DHT11Data;
  while (1)
  {

    uint8_t Temp = 0;
    uint8_t hum = 0;
	if(0 == DHT11_Read_Data(&Temp,&hum))
	{
		if(0 == Temp &&  0 == hum)
		{
	
		}else
		{
			Get_DHT11Data.temp = Temp;
			Get_DHT11Data.humidity = hum; 
     

			xQueueSend(REDHT11_Queue, &Get_DHT11Data, 0); /* code */
		}
			
	}		
   

  }
}
void DHT11_GETDATA_Thread_Start(void *PThread, StackType_t *PThread_Stack, int LMotor_Stack)
{
  xTaskCreateStatic(DHT11_GETDATA_Thread,
                    "DHT11",
                    LMotor_Stack,
                    NULL,
                    (osPriority_t)osPriorityNormal - 1,
                    PThread_Stack,
                    PThread);
}
