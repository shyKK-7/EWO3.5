#ifndef __STOP_KEY_H__
#define __STOP_KEY_H__
#include "stm32f4xx.h"                  // Device header
#include "./usart/bsp_usart.h"

typedef struct{
    u8 start_key_state;					//该电机起点限位开关的状态,等于3为按下，等于0为未按下
	u8 end_key_state;					//该电机终点限位开关的状态,等于3为按下，等于0为未按下
	u16 cnt1;					//该电机按键1的计时
	u16 cnt2;					//该电机按键2的计时
	u8 bit1;				//该电机按键1运行时的状态
	u8 bit2;				//该电机按键2运行时的状态
}KEY_STATE;

extern KEY_STATE Step1_Key, Step2_Key, TakeEgg_Key;

//按键定时器
#define Step_Key_TIM_CLK					RCC_APB1Periph_TIM4
#define Step_Key_TIM							TIM4		
#define Step_Key_TIM_Period				100-1	
#define Step_Key_TIM_Prescaler		4800-1          //APB1 48M APB2 84M/168
#define Step_Key_TIM_IRQn		      TIM4_IRQn

//按键GPIO
#define Step_Key_CLK					RCC_AHB1Periph_GPIOD	
#define Step_Key_port					GPIOD
/*限位开关引脚定义    1为起点限位开关 2为终点限位开关*/
#define Step1_Key_1_Pin				GPIO_Pin_1			
#define Step1_Key_2_Pin				GPIO_Pin_2		
#define TakeEgg_Key_0_Pin             GPIO_Pin_3

//#define Step2_Key_1_Pin				GPIO_Pin_2			
//#define Step2_Key_2_Pin				GPIO_Pin_3			
//#define Step3_Key_1_Pin				GPIO_Pin_4			
//#define Step3_Key_2_Pin				GPIO_Pin_5			
//#define Step4_Key_1_Pin				GPIO_Pin_6			
//#define Step4_Key_2_Pin				GPIO_Pin_7

#define KEY_NOTPRESSED      0       //没有按下    
#define KEY_PRESSED         1       //按住
#define KEY_LONGPRESS1      2       //长按2s
#define KEY_SHORTPRESS      3       //短按按2s

void Stop_Key_Init(void);

/********************************
使用方法:
	等于3时按下
	
	eg.if(Mykey.key_1 == 3)
		{
			
		}
********************************/
#endif
