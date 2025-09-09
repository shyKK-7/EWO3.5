#ifndef __HX711_H
#define	__HX711_H

#include "stm32f4xx.h"                  // Device header
#include "string.h"
#include "./delay/delay.h"
#include "./usart/bsp_usart.h"
#include "./usart_lcd/usart_lcd.h"
#include <easyflash.h>
#include "ef_types.h"
#include "./struct/struct.h"

extern struct device dev;

#define    HX711_PORT                 GPIOC

#define    Hx711_GPIO_CLK             RCC_AHB1Periph_GPIOC	  

//  SCK引脚定义
#define    HX711_SCK_PIN	          GPIO_Pin_2

//  DOUT引脚定义	   
#define    HX711_DOUT_PIN	 	      GPIO_Pin_3

#define    STABLE_TIMES                7//获取稳定重量的次数，间隔100ms
#define    INTERVAL_TIME               100//每次获取重量的时间间隔
  

#define HX711_DOUT GPIO_ReadInputDataBit(HX711_PORT, HX711_DOUT_PIN)

#define HX711_SCK(x) GPIO_WriteBit(HX711_PORT, HX711_SCK_PIN, (BitAction)x)

#define HX711_SAVE_ADDR 3   //第510块扇区存放校准值,第一个字节存放是否校准0X0A,后面依次存放x偏移参数xfac，y偏移参数yfac，x偏移量xoff, y偏移量yoff
#define HX711_SAVE_VALUE 0X0A  //已校准的标志符

#define SAMPLES 7           //中值滤波采样次数

extern u32 HX711_Buffer;
extern s32 Weight_before;

void Hx711_Config(void);
u32 HX711_Read(void);
void Get_Maopi(void);
s32 Get_Weight(void);
void Get_tare(void);//
void Hx711_Init(void);
s32 Get_StableWeight(void);//
void Hx711_AdjustParameter(void);
u32 Hx711_Read_MedianValue_adj(void);

#endif /* __HX711_H */

