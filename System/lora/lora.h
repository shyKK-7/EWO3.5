#ifndef __LORA_H
#define	__LORA_H

#include "stm32f4xx.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "./struct/struct.h"
extern QueueHandle_t Lora_Buffer_Store_QueueHandle_t;//数据存储队列
extern QueueHandle_t Lora_Buffer_Send_QueueHandle_t;//数据发送队列
extern QueueHandle_t Lora_Buffer_LastSend_QueueHandle_t;//上次一发送的内容，用于校验

/*串口中断的接收缓存*/
typedef struct Lora_Usart {
    char USART_BUFF[200];
    int USART_Length;
}Lora_Usart;
extern Lora_Usart Lora_Struct;

extern char Lora_Flag;//接收到唤醒指令Lora置1

/*Lora将要发送的状态的枚举*/
typedef enum State
{
    Free = 0,//空闲
    GetTime = 1,//校验时间
    Sendegg = 2,//产蛋数据
    Error = 3,//错误
    Live = 4,//心跳
    TakeTime = 5,//取蛋时间
}State;
extern enum Lora_State Lora_Station;

typedef struct Lora_Buffer//
{
    u8 id1[13];//左脚rfid的id
    u8 id2[13];//右脚rfid的id
    u32 enter_time;//进窝的时间
    u32 leave_time;//离开窝的时间
    u32 weight;//重量
    u8 cnt ;//发送的次数
//    DEVERROR error ;//错误的类型
    State station;//判断发送的类型
}Lora_Buffer;
extern Lora_Buffer takeegg_buffer;

//用于存储flash，应对掉电
typedef struct Lora_Array
{
    Lora_Buffer Array_date[20];
    int Array_Length;
}Lora_Array;

extern char Lora_NID[10];//存放LoraID

#define USART3_MAX_SEND_LEN 200//最大缓存两


/*******************************************************/
#define LORA_RES_PIN                  GPIO_Pin_0                
#define LORA_RES_GPIO_PORT            GPIOD                     
#define LORA_RES_GPIO_CLK             RCC_AHB1Periph_GPIOD

#define LORA_UART                             USART3
#define LORA_UART_CLK                         RCC_APB1Periph_USART3
#define LORA_UART_BAUDRATE                    115200  

#define LORA_UART_RX_GPIO_PORT                GPIOB
#define LORA_UART_RX_GPIO_CLK                 RCC_AHB1Periph_GPIOB
#define LORA_UART_RX_PIN                      GPIO_Pin_11
#define LORA_UART_RX_AF                       GPIO_AF_USART3
#define LORA_UART_RX_SOURCE                   GPIO_PinSource11

#define LORA_UART_TX_GPIO_PORT                GPIOB
#define LORA_UART_TX_GPIO_CLK                 RCC_AHB1Periph_GPIOB
#define LORA_UART_TX_PIN                      GPIO_Pin_10
#define LORA_UART_TX_AF                       GPIO_AF_USART3
#define LORA_UART_TX_SOURCE                   GPIO_PinSource10

#define LORA_UART_IRQHandler                  USART3_IRQHandler
#define LORA_UART_IRQ                 		  USART3_IRQn
/************************************************************/

void Lora_Res_Config(void);//复位引脚初始化
void Lora_Uart_Config(void);//串口初始化
void Lora_Init(void);//lora配置初始化
void Lora_Uart_SendByte( USART_TypeDef * pLorax, uint8_t ch);
void Lora_Uart_SendString( USART_TypeDef * pLorax, char *str);
void Lora_Uart_SendHalfWord( USART_TypeDef * pLorax, uint16_t ch);
void Lora_Clear_Struct(void);
void Lora_printf(char* fmt,...);

void Lora_Wakeup(void);//唤醒
void Lora_EnterAT(void);//进入at模式
void Lora_GetID(void);//获取id增加代码适用性，因为每个lora模块的id不一样
void Lora_SetAT(void);//配置模式

void Lora_CLear_Lora_buffer(Lora_Buffer *temp);
void Lora_Write_takeegg_buffer(Lora_Buffer temp);
void Lora_Read_takeegg_buffer(Lora_Buffer *temp);

void Lora_Array_clear(void);
void Lora_Flash_to_queue(void);//从flash读出
void Lora_Buffer_Store_to_Flash(void);//存入flash
void Lora_Buffer_Send_to_Flash(void);//存入flash
void Lora_Buffer_LastSend_to_Flash(void);//存入flash



#endif /* __USART1_H */
