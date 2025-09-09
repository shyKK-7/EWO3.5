
#include "./usart/bsp_usart.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "String.h"
#include "./RTC/bsp_rtc.h"
#include "./iwdg/iwdg.h"
extern SemaphoreHandle_t Usart1_mutex;
char usart1_buf[BUF_LENGTH];
u8 usart1_bufcnt;

void Usart1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//1) 串口时钟和 GPIO 时钟使能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//串口时钟UART1使能,串口是挂载在 APB2 下面的外设
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);//GPIO 时钟使能, 串口1 GPIO对应芯片引脚PA9,PA10
	
	//2) 设置引脚复用器映射
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	
	//3) GPIO 端口模式设置：PA9 和 PA10 要设置为复用功能
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;//GPIOA9 与 GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;//速度 50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化 PA9，PA10
	
	//4) 串口参数初始化：设置波特率，字长，奇偶校验等参数
	USART_InitStructure.USART_BaudRate = 115200;//波特率设置为9600
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控制选择无
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//收发模式
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为 8 位数据格式
	USART_Init(USART1, &USART_InitStructure); //初始化串口1

	
	//5) 开启中断并且初始化 NVIC，使能相应中断
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);//开启中断，接收到数据中断
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//串口1中断通道,在顶层头文件stm32f4xx.h中第223行定义
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//IRQ 通道使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//抢占优先级 3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//响应优先级 3
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化NVIC寄存器

	//6) 使能串口接收中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ITConfig (USART1, USART_IT_IDLE, ENABLE ); //使能串口总线空闲中断 	

	//7) 使能串口
	USART_Cmd(USART1, ENABLE);  //使能串口1
}

void Usart_SendByte(USART_TypeDef * pUSARTx, uint8_t Byte)
{
    USART_SendData(pUSARTx, Byte);
    /* 等待发送完成 */
	while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET);
}

void Usart1_Printf(char* fmt, ...)    //无法列出传递函数的所有实参的类型和数目时,可以用省略号指定参数表
{
    
    u16 i, j;
    if (xSemaphoreTake(Usart1_mutex, portMAX_DELAY) == pdTRUE) 
    {
        va_list ap;          //va_list 是一个字符指针，可以理解为指向当前参数的一个指针，取参必须通过这个指针进行。
        va_start(ap, fmt);   //va_start函数来获取参数列表中的参数，使用完毕后调用va_end()结束
        vsprintf((char*)usart1_buf, fmt, ap);	// 把生成的格式化的字符串存放在这里
        va_end(ap);
        i = strlen((const char*)usart1_buf);              //此次发送数据的长度
        for(j = 0; j < i; j++)                                                    //循环发送数据
        {
            while((USART1->SR & 0X40) == 0);                    //循环发送,直到发送完毕
            USART1->DR = usart1_buf[j];
        }
        xSemaphoreGive(Usart1_mutex);
    }
}
int fputc(int ch, FILE *f)
{
    Usart_SendByte(USART1, ch);
    return ch;
}

void easyflash_delrecover(void)
{
    ef_del_env("devstate");
    ef_del_env("flag_eggadd");
    ef_del_env("queue0");
    ef_del_env("queue1");
    ef_del_env("queue2");
    ef_del_env("queue3");
    ef_del_env("queue4");
    ef_del_env("queue5");
    ef_del_env("time_nesting");
    ef_del_env("Lora_Buffer_Store");
    ef_del_env("Lora_Buffer_Send");
    ef_del_env("Lora_Buffer_LastSend");
}

void USART1_IRQHandler()
{
    if(USART_GetITStatus(USART1,USART_IT_RXNE))
    {
		usart1_buf[usart1_bufcnt] = USART_ReceiveData(USART1);
		usart1_bufcnt++;
        while(USART_GetITStatus(USART1, USART_IT_RXNE) == SET);
    }
    if(USART_GetITStatus(USART1,USART_IT_IDLE))
	{
        /*串口接收处理*/
        const char *resethx711 = "resethx711";
		const char *delrecover = "delrecover";
//        const char *timeset = "RTCTIMESET";
		//清除中断标志位
		USART1->SR;
		USART1->DR;
		usart1_buf[usart1_bufcnt] = '\0';
        if (!memcmp(usart1_buf, resethx711, strlen(resethx711)))
        {
            ef_del_env("HX711_Parameter");
        }
        if (!memcmp(usart1_buf, delrecover, strlen(delrecover)))
        {
            printf("easyflash_delrecover\n");
            IWDG_Feed();
            easyflash_delrecover();
        }
//		if (!memcmp(usart1_buf, reset, strlen(reset)))
//		{
//			/*重置断电恢复标志位*/
//			Flash_ClearCheck();
//		}
//        if (!memcmp(usart1_buf, timeset, strlen(timeset)))
//		{
//            /*格式：RTCTIMESET 24 08 13 2 23 25 00 */
//            printf("USART RTC SET TIME\n");
//			/*取出时间信息*/
//            u8 year, month, date, day, hour, min, sec;
//            year = (usart1_buf[11]-'0')*10+(usart1_buf[12]-'0');
//            month = (usart1_buf[14]-'0')*10+(usart1_buf[15]-'0');
//            date = (usart1_buf[17]-'0')*10+(usart1_buf[18]-'0');
//            day = (usart1_buf[20]-'0');
//            hour = (usart1_buf[22]-'0')*10+(usart1_buf[23]-'0');
//            min = (usart1_buf[25]-'0')*10+(usart1_buf[26]-'0');
//            sec = (usart1_buf[28]-'0')*10+(usart1_buf[29]-'0');
//            MyRTC_SetTime(year, month, date, day, hour, min, sec);
//		}
        memset(usart1_buf, 0, BUF_LENGTH);      
        usart1_bufcnt = 0;
	}
}

