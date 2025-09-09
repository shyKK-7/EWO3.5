#include "./stop_key/stop_key.h"

//定义四个电机的按键，每个电机有两个按键
KEY_STATE Step1_Key, Step2_Key, TakeEgg_Key;


/********************************************************************************/
void Stop_Key_TIM_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitSturcture;

    RCC_APB1PeriphClockCmd(Step_Key_TIM_CLK, ENABLE);           //使能定时器

    /* 配置TIM4 */
    TIM_TimeBaseInitStructure.TIM_Period = Step_Key_TIM_Period;                     //自动装载
    TIM_TimeBaseInitStructure.TIM_Prescaler = Step_Key_TIM_Prescaler;                 //预分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;		//递增计数
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(Step_Key_TIM, &TIM_TimeBaseInitStructure);								//初始化

    TIM_ITConfig(Step_Key_TIM, TIM_IT_Update, ENABLE);                     		//使能更新中断

    /* NVIC配置 */
    NVIC_InitSturcture.NVIC_IRQChannel = Step_Key_TIM_IRQn;                		//中断请求
    NVIC_InitSturcture.NVIC_IRQChannelPreemptionPriority = 0;      		//抢占优先级1
    NVIC_InitSturcture.NVIC_IRQChannelSubPriority = 0;             		//响应优先级1
    NVIC_InitSturcture.NVIC_IRQChannelCmd = ENABLE;                		//使能中断
    NVIC_Init(&NVIC_InitSturcture);                                		//初始化NVIC

    TIM_Cmd(Step_Key_TIM, ENABLE);                                         		//使能
}

/********************************************************************************/
void Stop_Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(Step_Key_CLK,ENABLE);//使能时钟
	
	/*****************key0 key2 key3 key4*******************/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 	
	//GPIO_InitStructure.GPIO_Pin = Step1_Key_1_Pin | Step2_Key_1_Pin | Step3_Key_1_Pin | Step4_Key_1_Pin;
    GPIO_InitStructure.GPIO_Pin = Step1_Key_1_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_Key_port, &GPIO_InitStructure);
	
	/*****************key0 key2 key3 key4*******************/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 	
	//GPIO_InitStructure.GPIO_Pin = Step1_Key_2_Pin | Step2_Key_2_Pin | Step3_Key_2_Pin | Step4_Key_2_Pin;
    GPIO_InitStructure.GPIO_Pin = Step1_Key_2_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_Key_port, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 	
	//GPIO_InitStructure.GPIO_Pin = Step1_Key_2_Pin | Step2_Key_2_Pin | Step3_Key_2_Pin | Step4_Key_2_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 
    GPIO_InitStructure.GPIO_Pin = TakeEgg_Key_0_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_Key_port, &GPIO_InitStructure);
	Stop_Key_TIM_Init();//初始化按键时钟
}
void TIM4_IRQHandler(void)
{
    if(TIM_GetITStatus(Step_Key_TIM,TIM_IT_Update) == SET) 
    {		/****************************************Step1_key_1********************************************************/
        switch (Step1_Key.bit1)
        {
            case 0:         
                /*未按下*/
                if (GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0 && Step1_Key.start_key_state == KEY_NOTPRESSED) 
                {
                    Step1_Key.bit1++;
                }
                break;
            case 1:    
                /*前消抖*/
                if (GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0)
                {
                    Step1_Key.bit1++;
                }
                break;
            case 2:         
                /*按住*/
                if (GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0)
                {
                    Step1_Key.cnt1++;
                    Step1_Key.start_key_state = KEY_PRESSED;
                }
                else
                {
                    //若需要长按计时在此添加
                    Step1_Key.start_key_state = KEY_NOTPRESSED;
                    Step1_Key.bit1++;       //消除后抖
                }
                break;
            case 3:    
                /*后消抖*/
                Step1_Key.bit1 = 0;
                Step1_Key.cnt1 = 0;
                break;
        }
        switch (Step1_Key.bit2)
        {
            case 0:         
                /*未按下*/
                if (GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0 && Step1_Key.end_key_state == KEY_NOTPRESSED) 
                {
                    Step1_Key.bit2++;
                }
                break;
            case 1:    
                /*前消抖*/
                if (GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0)
                {
                    Step1_Key.bit2++;
                }
                break;
            case 2:         
                /*按住*/
                if (GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0)
                {
                    Step1_Key.cnt2++;
                    Step1_Key.end_key_state = KEY_PRESSED;
                }
                else
                {
                    printf("Key out\n");
                    printf("Step1_Key.cnt2:%d\n", Step1_Key.cnt2);
                    /*松开*/
                    if (Step1_Key.cnt2 > 200)   //长按2S，重新校准传感器
                    {
                        Step1_Key.end_key_state = KEY_LONGPRESS1;
                        printf("KEY_LONGPRESS1\n");
                    }
                    else
                    {
                        Step1_Key.end_key_state = KEY_NOTPRESSED;
                    }
                    Step1_Key.bit2++;    //消除后抖
                }
                break;
            case 3:    
                /*后消抖*/
                Step1_Key.bit2 = 0;
                Step1_Key.cnt2 = 0;
                break;
        }
         switch (TakeEgg_Key.bit1)
        {
            case 0:         
                /*未按下*/
                if (GPIO_ReadInputDataBit(Step_Key_port,TakeEgg_Key_0_Pin) == 0 && TakeEgg_Key.start_key_state == KEY_NOTPRESSED) 
                {
                    TakeEgg_Key.bit1++;
                }
                break;
            case 1:    
                /*前消抖*/
                if (GPIO_ReadInputDataBit(Step_Key_port,TakeEgg_Key_0_Pin) == 0)
                {
                    TakeEgg_Key.bit1++;
                }
                break;
            case 2:         
                /*按住*/
                if (GPIO_ReadInputDataBit(Step_Key_port,TakeEgg_Key_0_Pin) == 0)
                {
                    TakeEgg_Key.cnt1++;
                    TakeEgg_Key.start_key_state = KEY_PRESSED;
                }
                else
                {
                    printf("Key out\n");
                    printf("TakeEgg_Key.cnt1:%d\n", TakeEgg_Key.cnt1);
                    if (TakeEgg_Key.cnt1 > 200)   //长按2S，重新校准传感器
                    {
                        TakeEgg_Key.start_key_state = KEY_LONGPRESS1;
                        printf("KEY_LONGPRESS1\n");
                    }
                    else
                    {
                        TakeEgg_Key.start_key_state = KEY_SHORTPRESS;
                    }
                    TakeEgg_Key.bit1++;       //消除后抖
                }
                break;
            case 3:    
                /*后消抖*/
                TakeEgg_Key.bit1 = 0;
                TakeEgg_Key.cnt1 = 0;
                break;
        }
    }
    TIM_ClearITPendingBit(Step_Key_TIM,TIM_IT_Update);
}

