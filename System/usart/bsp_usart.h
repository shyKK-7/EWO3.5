#ifndef __BSP_USART_H
#define __BSP_USART_H

#define __DEBUG__           //把这行注释掉相当于等于注释掉工程中所有DEBUG打印
#ifdef  __DEBUG__
        #define DEBUG(...) printf(__VA_ARGS__)  //宏打印函数定义
    #else
        #define DEBUG(...)
    #endif



#include "stm32f4xx.h"                  // Device header
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "easyflash.h"

#define BUF_LENGTH 256
//#include "FreeRTOS.h"
//#include "FreeRTOSConfig.h"
//#include "task.h"
//#include "semphr.h"
//#include "String.h"

//extern SemaphoreHandle_t Usart1_mutex;

void Usart1_Init(void);
void Usart_SendByte(USART_TypeDef * pUSARTx, uint8_t Byte);
void Usart1_Printf(char* fmt, ...);
void Usart1_Printf_ISR(char* fmt, ...) ;

#endif
