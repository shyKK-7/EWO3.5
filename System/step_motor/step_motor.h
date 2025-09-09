#ifndef __STEP_MOTOR_H
#define __STEP_MOTOR_H

#include "stm32f4xx.h"                  // Device header


//步进电机定时器
#define Step_TIM_CLK 					RCC_APB1Periph_TIM2
#define Step_TIM 					    TIM2
#define Step_TIM_IRQChannel 	TIM2_IRQn
#define Step_TIM_Period				168	    //ARR
#define Step_TIM_Prescaler 		104			//PSC


//步进电机时钟
#define Step_PUL_CLK 					RCC_AHB1Periph_GPIOA
#define Step_DIR_CLK 					RCC_AHB1Periph_GPIOC
#define Step_ENA_CLK 					RCC_AHB1Periph_GPIOC

//步进电机的GPIO
#define Step_PUL_port 				GPIOA
#define Step_DIR_port 				GPIOC
#define Step_ENA_port 				GPIOC

//步进电机1
#define Step1_Pul_Pin 				GPIO_Pin_0
#define Step1_DIR_Pin 				GPIO_Pin_0
#define Step1_ENA_Pin 				GPIO_Pin_1

//步进电机2
#define Step2_Pul_Pin 				GPIO_Pin_1
#define Step2_DIR_Pin 				GPIO_Pin_2
#define Step2_ENA_Pin 				GPIO_Pin_3

//步进电机3
#define Step3_Pul_Pin 				GPIO_Pin_2
#define Step3_DIR_Pin 				GPIO_Pin_4
#define Step3_ENA_Pin 				GPIO_Pin_5

//步进电机4
#define Step4_Pul_Pin 				GPIO_Pin_3
#define Step4_DIR_Pin 				GPIO_Pin_6
#define Step4_ENA_Pin 				GPIO_Pin_7

//步进电机方向控制
#define 	Step_DIR_Forward 	    Bit_SET             //顺时针
#define 	Step_DIR_Counter 	    Bit_RESET               //逆时针

//步进电机使能控制
#define 	Step_ENA_ON 	        Bit_RESET               //开启
#define 	Step_ENA_OFF 		   	  Bit_SET               //关闭

//步进电机的选择
typedef enum {
	Step1,                          
	Step2,                         
	Step3,
	Step4
}Step;

enum MOTORSTATE
{
    STOP  = 0, 
    MOVE
};
struct motor
{
    enum MOTORSTATE state;
    void(*ON)(BitAction);
    void(*OFF)(void);
    void(*Move)(void);
};

void STEP_Init(void);
void Step_ON(Step Step_num, BitAction Step_DIR);
void Step_OFF(Step Step_num);
void Step1_ON(BitAction Step_DIR);
void Step1_OFF(void);
void Step1_Move(void);

extern struct motor step1;




#endif
