#ifndef __RFID_R200_H__
#define __RFID_R200_H__

#include "stm32f4xx.h"
#include <String.h>	
#include "./usart/bsp_usart.h"
#include "./delay/delay.h"
#include "./struct/struct.h"


#if 0
    #define RFID_R200
#else
    #define RFID_PANEL
#endif

//RFID模块用到串口2
//要改用其他串口就在这里改

#define  RFID_USARTx                   USART2
#define  RFID_USART_CLK                RCC_APB1Periph_USART2
#define  RFID_USART_APBxClkCmd         RCC_APB1PeriphClockCmd
#define  RFID_USART_BAUDRATE           115200

//// USART GPIO 引脚宏定义
#define  RFID_USART_GPIO_CLK           (RCC_AHB1Periph_GPIOA)
#define  RFID_USART_GPIO_APBxClkCmd    RCC_AHB1PeriphClockCmd
    
#define  RFID_USART_TX_GPIO_PORT       GPIOA   
#define  RFID_USART_TX_GPIO_PIN        GPIO_Pin_2
#define  RFID_USART_RX_GPIO_PORT       GPIOA
#define  RFID_USART_RX_GPIO_PIN        GPIO_Pin_3

#define  RFID_USART_IRQ                USART2_IRQn
#define  RFID_USART_IRQHandler         USART2_IRQHandler


//大鹅项目RFID读卡可能用到的参数
#define BUF_LENGTH 256
#define CARD_MAX 6
#define goose_ID_cnt 25			//读到单个大鹅脚环返回的数据数
#define ID_OFFSET 8             //ID的偏移字节，即ID前有几个字节
#define ID_LENGTH 12              //ID的长度
#define fail_cnt 8				//无卡时返回的数据数
#define find_cmd_cnt 10			//读卡指令的字节数（单次多次都一样）
#define RFID_R200_2D_LENGTH 10	//双通道天线初始化的字节数
#define RFID_R200_4D_LENGTH 12	//四通道天线初始化的字节数



//读卡器标识宏定义
#define HF2channel 0x02
#define HF4channel 0x04




#define USART_RBUFF_SIZE 1024

struct RFID_ID
{
    u8 id[ID_LENGTH];       //卡号
    u32 cnt;                //卡号被读到的次数
    u32 notcnt;             //卡号没被读到的次数
};

extern struct RFID_ID RFID_ID_buf[CARD_MAX];
extern u8 card_cnt;
extern u8 empty_id[ID_LENGTH];

void RFID_Init(void);
void RFID_Read(void);
void RFID_Heartbreat(void);
void RFID_CleanIDBuf(void);
void RFID_NeglectID(u8 *id);


#endif		//__RFID_R200_H__
