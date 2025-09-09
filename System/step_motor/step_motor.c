#include "./step_motor/step_motor.h"                  // Device header
#include "./usart/bsp_usart.h"
#include "./delay/delay.h"

struct motor step1 = 
{
    .state = STOP,                 // 初始化state值
    .ON = Step1_ON,             // 赋值函数指针
    .OFF = Step1_OFF,            // 赋值函数指针
    .Move = Step1_Move
};
void STEP_Init(void)			
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(Step_TIM_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd(Step_PUL_CLK | Step_DIR_CLK | Step_ENA_CLK, ENABLE);
	
	//脉冲
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_TIM2);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	//GPIO_InitStructure.GPIO_Pin = Step1_Pul_Pin | Step2_Pul_Pin | Step3_Pul_Pin | Step4_Pul_Pin;
    GPIO_InitStructure.GPIO_Pin = Step1_Pul_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_PUL_port, &GPIO_InitStructure);
	
	//方向
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	//GPIO_InitStructure.GPIO_Pin = Step1_DIR_Pin | Step2_DIR_Pin | Step3_DIR_Pin | Step4_DIR_Pin;
    GPIO_InitStructure.GPIO_Pin = Step1_DIR_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(Step_DIR_port, &GPIO_InitStructure);
	
	//使能
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	//GPIO_InitStructure.GPIO_Pin = Step1_ENA_Pin | Step2_ENA_Pin | Step3_ENA_Pin | Step4_ENA_Pin;
    GPIO_InitStructure.GPIO_Pin = Step1_ENA_Pin;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_ENA_port, &GPIO_InitStructure);
		
	TIM_InternalClockConfig(Step_TIM);//pwm时钟使能
	
	//时钟配置
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;				
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;		//向上计数
	TIM_TimeBaseInitStructure.TIM_Period = Step_TIM_Period - 1;					 //ARR
	TIM_TimeBaseInitStructure.TIM_Prescaler = Step_TIM_Prescaler - 1;		 //PSC
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(Step_TIM, &TIM_TimeBaseInitStructure);
	
	//pwm配置
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;		//CCR
	
	TIM_ClearFlag(Step_TIM, TIM_FLAG_Update);
	TIM_ITConfig(Step_TIM,TIM_IT_Update,ENABLE ); 
	
	//中断设置
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);  
	
	//使能通道
	TIM_OC1Init(Step_TIM, &TIM_OCInitStructure);
//	TIM_OC2Init(Step_TIM, &TIM_OCInitStructure);
//	TIM_OC3Init(Step_TIM, &TIM_OCInitStructure);
//	TIM_OC4Init(Step_TIM, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(Step_TIM, TIM_OCPreload_Enable); 
//	TIM_OC2PreloadConfig(Step_TIM, TIM_OCPreload_Enable);
//	TIM_OC3PreloadConfig(Step_TIM, TIM_OCPreload_Enable); 
//	TIM_OC4PreloadConfig(Step_TIM, TIM_OCPreload_Enable);	
	TIM_Cmd(Step_TIM, ENABLE);
	
	//全部电机关闭使能
	GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_OFF);
//	GPIO_WriteBit(Step_ENA_port,Step2_ENA_Pin,Step_ENA_OFF);
//	GPIO_WriteBit(Step_ENA_port,Step3_ENA_Pin,Step_ENA_OFF);
//	GPIO_WriteBit(Step_ENA_port,Step4_ENA_Pin,Step_ENA_OFF);
	  
}


/**************************
* 函数名称	Step_ON
* 参数			Step_num：Step1, Step2, Step3, Step4
* 参数			Step_DIR：Step_DIR_Forward, Step_DIR_Counter
* 返回值		无
* 功能			开电机
***************************/
void Step_ON(Step Step_num, BitAction Step_DIR)
{
	switch (Step_num) 
	{
		case Step1:
			GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_ON);
			GPIO_WriteBit(Step_DIR_port,Step1_DIR_Pin,Step_DIR);
			TIM_SetCompare1(Step_TIM, Step_TIM_Period / 2 - 1);
			break;
		case Step2:
			GPIO_WriteBit(Step_ENA_port,Step2_ENA_Pin,Step_ENA_ON);
			GPIO_WriteBit(Step_DIR_port,Step2_DIR_Pin,Step_DIR);
			TIM_SetCompare2(Step_TIM, Step_TIM_Period / 2 - 1);
			break;
		case Step3:
			GPIO_WriteBit(Step_ENA_port,Step3_ENA_Pin,Step_ENA_ON);
			GPIO_WriteBit(Step_DIR_port,Step3_DIR_Pin,Step_DIR);
			TIM_SetCompare3(Step_TIM, Step_TIM_Period / 2 - 1);
			break;
		case Step4:
			GPIO_WriteBit(Step_ENA_port,Step4_ENA_Pin,Step_ENA_ON);
			GPIO_WriteBit(Step_DIR_port,Step4_DIR_Pin,Step_DIR);
			TIM_SetCompare4(Step_TIM, Step_TIM_Period / 2 - 1);
			break;
		default:
			break;
	}
}


/**************************
* 函数名称	Step_OFF
* 参数			Step_num：Step1, Step2, Step3, Step4
* 参数			Step_DIR：Step_DIR_Forward, Step_DIR_Counter
* 返回值		无
* 功能			关电机
***************************/
void Step_OFF(Step Step_num)
{
	switch (Step_num) 
	{
		case Step1:
			GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_OFF);
			TIM_SetCompare1(Step_TIM, 0);
			break;
		case Step2:
			GPIO_WriteBit(Step_ENA_port,Step2_ENA_Pin,Step_ENA_OFF);
			TIM_SetCompare2(Step_TIM, 0);
			break;
		case Step3:
			GPIO_WriteBit(Step_ENA_port,Step3_ENA_Pin,Step_ENA_OFF);
			TIM_SetCompare3(Step_TIM, 0);
			break;
		case Step4:
			GPIO_WriteBit(Step_ENA_port,Step4_ENA_Pin,Step_ENA_OFF);
			TIM_SetCompare4(Step_TIM, 0);
			break;
		default:
			break;
	}
}
/**************************
* 函数名称	    Step1_ON
* 参数			方向Step_DIR：Step_DIR_Forward, Step_DIR_Counter     
* 返回值		无
* 功能			启动步进电机1
***************************/
void Step1_ON(BitAction Step_DIR)
{
    step1.state = MOVE;
    GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_ON);
    GPIO_WriteBit(Step_DIR_port,Step1_DIR_Pin,Step_DIR);
    TIM_SetCompare1(Step_TIM, Step_TIM_Period / 2 - 1);
}

/**************************
* 函数名称	Step1_OFF
* 参数			无
* 返回值		无
* 功能			关闭步进电机1
***************************/
void Step1_OFF(void)
{
    step1.state = STOP;
    GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_OFF);
    TIM_SetCompare1(Step_TIM, 0);
}

//Move即往前移动到终点，然后复位到起点
void Step1_Move(void)
{
    step1.state = MOVE;
}
void TIM2_IRQHandler(void)   
{
	if (TIM_GetITStatus(Step_TIM, TIM_IT_Update) != RESET) //中断是否发送
	{
		TIM_ClearITPendingBit(Step_TIM, TIM_IT_Update  );  //清除中断信号
	}
}






