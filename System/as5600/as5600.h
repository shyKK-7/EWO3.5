#ifndef _AS5600_H
#define _AS5600_H


#include <inttypes.h>
#include "stm32f4xx.h"                  // Device header
#include "./usart/bsp_usart.h"


#define	_raw_ang_hi 0x0c
#define	_raw_ang_lo 0x0d

#define AS5600_I2C_WR	0		/* 写控制bit */
#define AS5600_I2C_RD	1		/* 读控制bit */


/* 定义I2C总线连接的GPIO端口, 用户只需要修改下面4行代码即可任意改变SCL和SDA的引脚 */
#define AS5600_I2C_PORT				GPIOB			/* GPIO端口 */
#define AS5600_I2C_CLK 				RCC_AHB1Periph_GPIOB		/* GPIO端口时钟 */
#define AS5600_I2C_SCL_PIN		GPIO_Pin_6			/* 连接到SCL时钟线的GPIO */
#define AS5600_I2C_SDA_PIN		GPIO_Pin_7			/* 连接到SDA数据线的GPIO */


#define AS5600_I2C_SCL_1()  GPIO_SetBits(AS5600_I2C_PORT, AS5600_I2C_SCL_PIN)		/* SCL = 1 */
#define AS5600_I2C_SCL_0()  GPIO_ResetBits(AS5600_I2C_PORT, AS5600_I2C_SCL_PIN)		/* SCL = 0 */

#define AS5600_I2C_SDA_1()  GPIO_SetBits(AS5600_I2C_PORT, AS5600_I2C_SDA_PIN)		/* SDA = 1 */
#define AS5600_I2C_SDA_0()  GPIO_ResetBits(AS5600_I2C_PORT, AS5600_I2C_SDA_PIN)		/* SDA = 0 */

#define AS5600_I2C_SDA_READ()  GPIO_ReadInputDataBit(AS5600_I2C_PORT, AS5600_I2C_SDA_PIN)	/* 读SDA口线状态 */




void As5600_Init(void);
uint16_t AS5600_ReadTwoByte(u16 ReadAddr_hi,u16 ReadAddr_lo);
float AS5600_Read_Speed(void);
int AS5600_GetAngle(void);


#endif
