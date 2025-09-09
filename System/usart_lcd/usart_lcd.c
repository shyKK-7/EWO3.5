#include <stm32f4xx.h>
#include "./usart_lcd/usart_lcd.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "String.h"


char *page_name[] = {"init", "noegg", "qrcode", "error" , "takeegg", "adjust", "working"};

extern struct device dev;
extern SemaphoreHandle_t Usart_lcd_mutex;   //互斥量 

char usart_lcd_send_buf[LCD_BUF_LENGTH];         //串口发送缓冲区
char usart_lcd_rev_buf[LCD_BUF_LENGTH];         //串口接收缓冲区
u8 usart_lcd_react = 0;                     //响应标志位
u32 timeout_cnt = 0;                        //超时计数器
u8 usart_lcd_rev_buf_cnt;                                  //串口接收计数器
u8 page_now;                                //当前页面
u8 crc_flag;                                //crc校验标志位

u8 react_success[4] = {0X01, 0XFF, 0XFF, 0XFF};





void Usart_lcd_SendByte(USART_TypeDef * pUSARTx, uint8_t Byte)
{
    USART_SendData(pUSARTx, Byte);
    /* 等待发送完成 */
	while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET);
}

void Usart_lcd_Printf_OS(char* fmt, ...)    //无法列出传递函数的所有实参的类型和数目时,可以用省略号指定参数表
{
    
    u16 i, j;
    if (xSemaphoreTake(Usart_lcd_mutex, portMAX_DELAY) == pdTRUE) 
    {
        va_list ap;          //va_list 是一个字符指针，可以理解为指向当前参数的一个指针，取参必须通过这个指针进行。
        va_start(ap, fmt);   //va_start函数来获取参数列表中的参数，使用完毕后调用va_end()结束
        vsprintf((char*)usart_lcd_send_buf, fmt, ap);	// 把生成的格式化的字符串存放在这里
        va_end(ap);
        i = strlen((const char*)usart_lcd_send_buf);              //此次发送数据的长度
        for(j = 0; j < i; j++)                                                    //循环发送数据
        {
            while((UART4->SR & 0X40) == 0);                    //循环发送,直到发送完毕
            UART4->DR = usart_lcd_send_buf[j];
        }
        memset(usart_lcd_send_buf, 0, i);                       //清空缓冲区
        xSemaphoreGive(Usart_lcd_mutex);
    }
}
void Usart_lcd_Printf(char* fmt, ...)    //无法列出传递函数的所有实参的类型和数目时,可以用省略号指定参数表
{
    u16 i, j;
    va_list ap;          //va_list 是一个字符指针，可以理解为指向当前参数的一个指针，取参必须通过这个指针进行。
    va_start(ap, fmt);   //va_start函数来获取参数列表中的参数，使用完毕后调用va_end()结束
    vsprintf((char*)usart_lcd_send_buf, fmt, ap);	// 把生成的格式化的字符串存放在这里
    va_end(ap);
    i = strlen((const char*)usart_lcd_send_buf);              //此次发送数据的长度
    for(j = 0; j < i; j++)                                                    //循环发送数据
    {
        while((USART_LCD->SR & 0X40) == 0);                    //循环发送,直到发送完毕
        USART_LCD->DR = usart_lcd_send_buf[j];
    }
    memset(usart_lcd_send_buf, 0, i);                       //清空缓冲区
}
//单片机MODBUS_CRC16代码
static u8 InvertUint8(u8 data)
{
   int i;
   u8 newtemp8 = 0;
   for (i = 0; i < 8; i++)
   {
      if ( (data & (1 << i) ) != 0) newtemp8 |= (u8)(1 << (7 - i));
   }
   return newtemp8;
}
static u16 InvertUint16(u16 data)
{
   int i;
   u16 newtemp16 = 0;
   for (i = 0; i < 16; i++)
   {
      if ( (data & (1 << i) ) != 0) newtemp16 |= (u16)(1 << (15 - i));
   }
   return newtemp16;
}
u16 CRC16_MODBUS(u8* data, int lenth)
{
   int i;
   u16 wCRCin = 0xFFFF;
   u16 wCPoly = 0x8005;
   u16 wChar = 0;
   while (lenth > 0)
   {
      wChar = *data;
      data++;
      wChar = InvertUint8( (u8)wChar);
      wCRCin ^= (u16)(wChar << 8);
      for (i = 0; i < 8; i++)
      {
         if ((wCRCin & 0x8000) != 0)
         {
            wCRCin = (u16)( (wCRCin << 1) ^ wCPoly);
         }else
         {
            wCRCin = (u16)(wCRCin << 1);
         }
      }
      lenth=lenth-1;
   }
   wCRCin = InvertUint16(wCRCin);
   return (wCRCin);
}

//切换页面
void Usart_lcd_switch(DEVPAGE page)
{
    if (page_now != page)
    {
        printf("\npage %s\n", page_name[page]);
        Usart_lcd_Printf_OS("page %s\xff\xff\xff", page_name[page]);
    }
   
//    if(usart_lcd_react)
//    {
//        //执行成功
//        usart_lcd_react = 0;
//    }
//    else
//    {
//        //发送错误
//        dev.error = ERROR_LCD;
//    }
}
//更新二维码
void Usart_lcd_qrcode_update(const char *data)
{
    u16 crc;
    u8 buf[200] = {0};
    crc_flag = 1;
    Usart_lcd_switch(PAGE_QRCODE);       //切换到二维码页面
    Delay_ms(10);
    sprintf((char *)buf, "qr0.txt=\"%s\"", data);   //计算CRC
    crc = CRC16_MODBUS(buf, strlen((char *)buf));
    Usart_lcd_Printf_OS("qr0.txt=\"%s\"%c%c\x01\xFE\xFE\xFE", data, crc&0X00FF, (crc&0XFF00)>>8);
    Delay_ms(10);
    Usart_lcd_Printf_OS("vis qr0,1\xff\xff\xff");               //显示二维码
    Delay_ms(10);
    if(crc_flag)
    {
        //执行成功
        crc_flag = 0;
    }
    else
    {
        //重发
        Usart_lcd_Printf_OS("qr0.txt=\"%s\"%c%c\x01\xFE\xFE\xFE", data, crc&0X00FF, (crc&0XFF00)>>8);
    }
}

//显示蛋数量和当前索引
void Usart_lcd_qrcode_show_info(u8 current_index, u8 total_count)
{
    Usart_lcd_switch(PAGE_QRCODE);       //切换到二维码页面
    Delay_ms(10);
    Usart_lcd_Printf_OS("t0.txt=\"蛋 %d/%d\"\xff\xff\xff", current_index + 1, total_count);
    Delay_ms(10);
}
//显示error页面的取蛋警告
void Usart_lcd_error_takeegg(void)
{
    Usart_lcd_switch(PAGE_ERROR);       //切换到错误页面
    Delay_ms(10);
    Usart_lcd_Printf_OS("t0.txt=\"检测到取蛋行为异常\r\n请先按下屏幕旁边按钮\r\n请勿将蛋放回\"\xff\xff\xff");
}

//显示error页面的蛋槽满警告
void Usart_lcd_error_showfull(void)
{
    Usart_lcd_switch(PAGE_ERROR);       //切换到错误页面
    Delay_ms(10);
    Usart_lcd_Printf_OS("t0.txt=\"当前蛋槽已满\r\n按下屏幕旁边按钮\r\n开始取蛋\"\xff\xff\xff");
}
//显示error页面的致命错误警告
void Usart_lcd_error_showerror(void)
{
    Usart_lcd_switch(PAGE_ERROR);       //切换到错误页面
    Delay_ms(10);
    Usart_lcd_Printf_OS("t0.txt=\"发生致命错误\r\n请检查电机、读卡器、传感器\r\n或检查接线\r\n按下按钮恢复装置\"\xff\xff\xff");
}


//更新takeegg页面的当前蛋数
void Usart_lcd_takeegg_updateegg(struct egg *queue, int queue_length)
{
    struct tm *localTime;
    localTime = localtime(&queue[queue_length - 1].egg_outtime);
    Usart_lcd_switch(PAGE_TAKEEGG);       //切换到取蛋页面
    Delay_ms(100);
    Usart_lcd_Printf_OS("t1.txt=\"当前蛋槽有%d颗蛋\"\xff\xff\xff", queue_length);
    Delay_ms(100);
//    Usart_lcd_Printf_OS("t3.txt=\"上次产蛋活动时间:%04d/%02d/%02d-%02d:%02d:%d\r\n蛋重:%dg\"\xff\xff\xff", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec, queue[queue_length - 1].weight);
}
//在adjust页面说明
void Usart_lcd_adjust_adjust(void)
{
    Usart_lcd_switch(PAGE_ADJUST);       //切换到校准页面
}
//在adjust页面显示校准中
void Usart_lcd_adjust_adjusting(void)
{
    Usart_lcd_switch(PAGE_ADJUST);       //切换到校准页面
    Delay_ms(100);
    Usart_lcd_Printf_OS("t0.txt=\"校准中\"\xff\xff\xff");
}
//在adjust页面显示校准完成
void Usart_lcd_adjust_success(void)
{
    Usart_lcd_switch(PAGE_ADJUST);       //切换到校准页面
    Usart_lcd_Printf_OS("t0.txt=\"校准完成,请将砝码取出\"\xff\xff\xff");
}
void Usart_lcd_working(void)
{
    Usart_lcd_switch(PAGE_WORKING);       //切换到工作页面
}

void Usart_lcd_noegg(void)
{
    Usart_lcd_switch(PAGE_NOEGG);       //切换到无蛋页面
}

void Usart_lcd_beep(void)
{
    Usart_lcd_Printf_OS("beep 100\xff\xff\xff");
}

void Usart_lcd_updatetime(void)
{
    u32 timestamp;
    struct tm *localTime;
    timestamp = MyRTC_GetTimestamp();
    localTime = localtime(&timestamp);
    Usart_lcd_Printf_OS("noegg.date.txt=\"%04d/%02d/%02d-%02d:%02d:%d\"\xff\xff\xff", localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
}


//屏幕显示无信号，当断网时用
void Usart_lcd_shownosignal(void)
{
    Usart_lcd_Printf_OS("noegg.t1.txt=\"无信号\"\xff\xff\xff");
}
//屏幕取消显示无信号，当恢复网络时用
void Usart_lcd_showsignal(void)
{
    Usart_lcd_Printf_OS("noegg.t1.txt=\"\"\xff\xff\xff");
}
//通过发送sendme指令查看是否有返回
void Usart_lcd_heartbeat(void)
{
    static u8 error_cnt;                               //错误计数器
    Usart_lcd_Printf_OS("sendme\xff\xff\xff");
    Delay_ms(10);
    if (usart_lcd_react)
    {
        usart_lcd_react = 0;
        error_cnt = 0;
    }
    else
    {
        error_cnt++;
        if (error_cnt >= 10)
        {
            //发送错误
            dev.error = ERROR_LCD;
            printf("------------------------------ERROR_LCD\n");
            error_cnt = 0;
        }
    }
}

void USART_LCD_IRQHandler(void)
{
    if(USART_GetITStatus(USART_LCD,USART_IT_RXNE))
    {
		usart_lcd_rev_buf[usart_lcd_rev_buf_cnt] = USART_ReceiveData(USART_LCD);
		usart_lcd_rev_buf_cnt++;
        while(USART_GetITStatus(USART_LCD, USART_IT_RXNE) == SET);
    }
    if(USART_GetITStatus(USART_LCD,USART_IT_IDLE))
	{
        //清除中断标志位
		USART_LCD->SR;
		USART_LCD->DR;
        
        /*串口接收处理*/
        if (usart_lcd_rev_buf[0] <= 0X24 && usart_lcd_rev_buf[0] != 0X01)
        {
            printf("LCD error code:%X\n", usart_lcd_rev_buf[0]);
        }
        if (usart_lcd_rev_buf[0] == 0X66)
        {
            page_now = usart_lcd_rev_buf[1];      //记录当前页面
            usart_lcd_react = 1;
        }
        if (usart_lcd_rev_buf[0] == 0X09)
        {
            printf("CRC error");
            crc_flag = 0;
        }
		usart_lcd_rev_buf[usart_lcd_rev_buf_cnt] = '\0';
        memset(usart_lcd_rev_buf, 0, LCD_BUF_LENGTH);      
        usart_lcd_rev_buf_cnt = 0;
	}
}



void Usart_lcd_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//1) 串口时钟和 GPIO 时钟使能
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//串口时钟UART4使能,串口4是挂载在 APB1 下面的外设
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);//GPIO 时钟使能, 串口4 GPIO对应芯片引脚PC10,PC11
	
	//2) 设置引脚复用器映射
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4);
	
	//3) GPIO 端口模式设置：PC10,PC11 要设置为复用功能
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;//GPIOC10 与 GPIOC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;//速度 50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOC,&GPIO_InitStructure); //初始化 PC10,PC11
	
	//4) 串口参数初始化：设置波特率，字长，奇偶校验等参数
	USART_InitStructure.USART_BaudRate = 115200;//波特率设置为115200
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控制选择无
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//收发模式
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为 8 位数据格式
	USART_Init(USART_LCD, &USART_InitStructure); //初始化串口1

	
	//5) 开启中断并且初始化 NVIC，使能相应中断
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	USART_ITConfig(USART_LCD,USART_IT_RXNE,ENABLE);//开启中断，接收到数据中断
	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//串口4中断通道,在顶层头文件stm32f4xx.h中第223行定义
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//IRQ 通道使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//抢占优先级 3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//响应优先级 3
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化NVIC寄存器

	//6) 使能串口接收中断
	USART_ITConfig(USART_LCD, USART_IT_RXNE, ENABLE);
	USART_ITConfig (USART_LCD, USART_IT_IDLE, ENABLE ); //使能串口总线空闲中断 	

	//7) 使能串口
	USART_Cmd(USART_LCD, ENABLE);  //使能串口4
    
    Delay_ms(100);
   // Usart_lcd_switch(PAGE_INIT);       //切换到初始化页面
}
