/*
 * @Author: XQZ
 * @Date: 2024-01-05 12:02:45
 * @LastEditors: XQZ
 * @LastEditTime: 2024-01-05 12:25:17
 * @FilePath: \F4新建工程\System\Delay.h
 * @Description: 
 * 
 * Copyright (c) 2024 by XQZ, All Rights Reserved. 
 */
#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"                  // Device header
//#define SysCLK 100000000
//#define Systick_us 168
//void Delay_Init(void);
//void Delay_us(u16 us);
//void Delay_ms(u16 us);

//void Delay_Init(void);
void Delay_us(u32 nus);
void Delay_ms(u32 nms);
void Delay_xms(u32 nms);


#endif
