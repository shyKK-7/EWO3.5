#ifndef __USART_LCD_H
#define __USART_LCD_H



#include "stm32f4xx.h"                  // Device header
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include "./delay/delay.h"
#include "./struct/struct.h"
#include "./RTC/bsp_rtc.h"


#define LCD_BUF_LENGTH 256

#define USART_LCD_IRQHandler UART4_IRQHandler
#define USART_LCD UART4

//#define PAGE_INIT           0             //初始化页面
//#define PAGE_NOEGG          1             //无蛋页面
//#define PAGE_QRCODE         2             //二维码页面    
//#define PAGE_ERROR          3             //错误页面    
//#define PAGE_TAKEEGG        4             //取蛋页面
//#define PAGE_ADJUST         5             //传感器校准页面
//#define PAGE_WORKING        6             //工作中页面

typedef enum DEVPAGE
{
    PAGE_INIT = 0,
    PAGE_NOEGG,
    PAGE_QRCODE,
    PAGE_ERROR, 
    PAGE_TAKEEGG,
    PAGE_ADJUST,
    PAGE_WORKING
}DEVPAGE;


void Usart_lcd_Init(void);
void Usart_lcd_SendByte(USART_TypeDef * pUSARTx, uint8_t Byte);
void Usart_lcd_Printf_OS(char* fmt, ...);
void Usart_lcd_Printf(char* fmt, ...);
void Usart_lcd_switch(DEVPAGE page);
void Usart_lcd_qrcode_update(const char *data);
void Usart_lcd_qrcode_show_info(u8 current_index, u8 total_count);
void Usart_lcd_error_takeegg(void);
void Usart_lcd_error_showfull(void);
void Usart_lcd_error_showerror(void);
void Usart_lcd_takeegg_updateegg(struct egg *queue, int queue_length);
void Usart_lcd_adjust_adjust(void);
void Usart_lcd_adjust_adjusting(void);
void Usart_lcd_adjust_success(void);
void Usart_lcd_working(void);
void Usart_lcd_noegg(void);


void Usart_lcd_beep(void);
void Usart_lcd_updatetime(void);
void Usart_lcd_heartbeat(void);
void Usart_lcd_shownosignal(void);
void Usart_lcd_showsignal(void);

#endif
