#include "./lora/lora.h"
#include "./delay/delay.h"
#include "./usart/bsp_usart.h"
#include "stdarg.h"	 	 
#include "stdio.h"	 	 	 
#include "stdlib.h"
#include "time.h"
#include <String.h>

#include "easyflash.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "./RTC/bsp_rtc.h"


//QueueHandle_t Lora_Buffer_Store_QueueHandle_t;//数据存储队列
//QueueHandle_t Lora_Buffer_Send_QueueHandle_t;//数据发送队列
//QueueHandle_t Lora_Buffer_LastSend_QueueHandle_t;//数据上次发送队列

extern SemaphoreHandle_t Lora_mutex;

char Lora_Tx_Buf[USART3_MAX_SEND_LEN];//重定向缓冲

Lora_Usart Lora_Struct;//Lora接收数据缓冲

char Lora_Flag;//接收到唤醒指令Lora置1	

char Lora_NID[10];//存放LoraID
char Lora_GWID[10];//存放网关ID
char Lora_SetNID[20]="";//AT指令头部格式
char Lora_SetGWID[21]="";//AT指令头部格式
char Lora_GetTime_Buff[41]="{\"type\":\"1\",\"data\":null,\"nid\":\"";//获取网络时间指令格式

Lora_Buffer takeegg_buffer;

static Lora_Array Lora_Arrays;

/*中断优先级*/
static void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = LORA_UART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure);
}

/*Lora复位GPIO初始化*/
void Lora_Res_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd (LORA_RES_GPIO_CLK, ENABLE); 

    GPIO_InitStructure.GPIO_Pin = LORA_RES_PIN;	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 

    GPIO_Init(LORA_RES_GPIO_PORT, &GPIO_InitStructure);	
}

/*Lora_Uart初始化*/
void Lora_Uart_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_AHB1PeriphClockCmd(LORA_UART_RX_GPIO_CLK|LORA_UART_TX_GPIO_CLK,ENABLE);

    RCC_APB1PeriphClockCmd(LORA_UART_CLK, ENABLE);

    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = LORA_UART_RX_PIN  ;  
    GPIO_Init(LORA_UART_RX_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = LORA_UART_TX_PIN;
    GPIO_Init(LORA_UART_TX_GPIO_PORT, &GPIO_InitStructure);

    GPIO_PinAFConfig(LORA_UART_RX_GPIO_PORT,LORA_UART_RX_SOURCE,LORA_UART_RX_AF);

    GPIO_PinAFConfig(LORA_UART_TX_GPIO_PORT,LORA_UART_TX_SOURCE,LORA_UART_TX_AF);

    USART_InitStructure.USART_BaudRate = LORA_UART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(LORA_UART, &USART_InitStructure); 

    NVIC_Configuration();

    USART_ITConfig(LORA_UART, USART_IT_RXNE, ENABLE);

    USART_Cmd(LORA_UART, ENABLE);

}

/******Lora初始化**********/
void Lora_Init(void)
{
    Lora_Res_Config();//复位IO
    Lora_Uart_Config();//Uart初始化
    GPIO_ResetBits(LORA_RES_GPIO_PORT,LORA_RES_PIN);//拉低20ms复位
    Delay_ms(20);
    GPIO_SetBits(LORA_RES_GPIO_PORT,LORA_RES_PIN);
    Lora_Wakeup();//唤醒
    Lora_EnterAT();//进入AT模式
    Lora_GetID();//获取ID
    Lora_SetAT();//配置
    Lora_NID[8] = '\0';//去除NID的\n
    strcat(Lora_GetTime_Buff,Lora_NID);//与获取时间格式连接
    strcat(Lora_GetTime_Buff,"\"}");//形成获取时间指令
}

/******Lora发送一个字节**********/
void Lora_Send_Char(char temp)      
{        
    USART_SendData(LORA_UART,(u8)temp);        
    while(USART_GetFlagStatus(LORA_UART,USART_FLAG_TXE)==RESET);         
}    

/******Lora发送数组**********/
void Lora_Send_buff(char buf[],u32 len)     
{    
    u32 i;    
    for(i=0;i<len;i++)
    {	
        Lora_Send_Char(buf[i]);
    }
}
/******清除接收缓冲区**********/
void Lora_Clear_Struct(void)
{
    memset(Lora_Struct.USART_BUFF,0,Lora_Struct.USART_Length);
    Lora_Struct.USART_Length=0;
}

/******接收中断**********/
void LORA_UART_IRQHandler(void)
{
    uint8_t ch;
    if(USART_GetITStatus(LORA_UART,USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(LORA_UART,USART_IT_RXNE);
        ch = USART_ReceiveData(LORA_UART);
        Lora_Struct.USART_BUFF[Lora_Struct.USART_Length++] = ch;//放入缓冲区
        //Usart_SendByte(DEBUG_USART,Lora_Struct.USART_BUFF[Lora_Struct.USART_Length-1]);//用于调试查看				
    }
    if(USART_GetITStatus(LORA_UART,USART_IT_IDLE) != SET)
    {    
        if (strcmp(Lora_Struct.USART_BUFF, "Lora") == 0)//是否为唤醒指令
        {
            Lora_Clear_Struct();//清除接收缓冲
            Lora_Flag=1;//成功唤醒配合其他标志可发送
        }
        USART_ClearITPendingBit(LORA_UART,USART_IT_IDLE);
    }

}	

/**
  * @brief  发送AT指令
	* @param  cmd：发送的命令，reply：发送成功返回的命令，wait：等待的时间
  * @retval 返回1发送成功，0失败
  */
int Lora_SendCmd(char* cmd, char* reply, int wait)
{
    Lora_Clear_Struct();
    printf("[Lora_SendCmd] %s", cmd);
    Lora_Send_buff((char*)cmd, strlen(cmd));

    Delay_ms(wait);
    if (strcmp(reply, "") == 0)
    {
        return 0;
    }
    if (Lora_Struct.USART_Length != 0)
    {
        Lora_Struct.USART_BUFF[Lora_Struct.USART_Length] = '\0';

        if (strstr((char*)Lora_Struct.USART_BUFF, reply))
        {
            printf("return:%s\n", Lora_Struct.USART_BUFF);
            return 1;
        }
        else
        {  
            printf("return:%s\n", Lora_Struct.USART_BUFF);
            return 0;
        }  
    }  

    return 0;
}
/****唤醒******/
void Lora_Wakeup(void)  
{
    Lora_SendCmd("Lora\n","LoRa Start!",300);
}
/****进入AT******/
void Lora_EnterAT(void)  
{
    int ret = 0;

    ret=Lora_SendCmd("+++","a",300);
    if(ret==1)
        ret=Lora_SendCmd("a","+OK",50);
    if(ret==1)
        Lora_SendCmd("AT+E=OFF\n","OK",300);
}
/****获取ID******/
void Lora_GetID(void)
{
    int ret = 0;
    ret=Lora_SendCmd("AT+NID?\n","+NID",300);
    if(ret==1)
    {
        strncpy(Lora_NID,&(Lora_Struct.USART_BUFF[7]),9);		
    }
//    ret=Lora_SendCmd("AT+GWID?\n","+GWID",300);
//    if(ret==1)
//    {
//        strncpy(Lora_GWID,&(Lora_Struct.USART_BUFF[8]),9);
//    }
    strcpy(Lora_SetNID,"AT+NID=");
    //strcpy(Lora_SetGWID,"AT+GWID=");
    strcat(Lora_SetNID,Lora_NID);
    //strcat(Lora_SetGWID,Lora_GWID);
    //	printf("Lora_SetNID:%s\n",Lora_SetNID);

    //	printf("Lora_SetGWID:%s\n",Lora_SetGWID);

}
/****配置******/
void Lora_SetAT(void)
{
    int ret = 0;
	ret=Lora_SendCmd("AT+E=OFF\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd(Lora_SetNID,"OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+LORAPROT=LG210\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+WMODE=NET\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+GWID=20866974\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+FEC=OFF\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+PNUM=0\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+SPD1=7\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+SPD2=8\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+CH1=4700\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+CH2=4800\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+PWR=22\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+CAD=ON\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+MCU=Lora,ascii\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+MFLAG=ON\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+UARTFT=10\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+UART=115200,8,1,NONE,NFC\r\n","OK",300);
	if(ret==1)
	ret=Lora_SendCmd("AT+Z\r\n","OK",300);
//	if(ret==1)
//	ret=Lora_SendCmd("[TX]:AT+ENTM\r\n","OK",300);
	memset(Lora_Struct.USART_BUFF,0,50);
	Lora_Struct.USART_Length=0;
}



/****重定向******/
void Lora_printf(char* fmt,...) 
{  
    u16 i,j;
		if (xSemaphoreTake(Lora_mutex, portMAX_DELAY) == pdTRUE) 
    {
			va_list ap; 
			va_start(ap,fmt); 
			vsprintf((char*)Lora_Tx_Buf,fmt,ap); 
			va_end(ap);
			i=strlen((const char*)Lora_Tx_Buf);		
			for(j=0;j<i;j++)							
			{
					while(USART_GetFlagStatus(LORA_UART,USART_FLAG_TC)==RESET); 
					USART_SendData(LORA_UART,Lora_Tx_Buf[j]);  
			} 
		  xSemaphoreGive(Lora_mutex);
    }
}

/****清除队列元素的数据******/
void Lora_CLear_Lora_buffer(Lora_Buffer *temp)
{
    temp->cnt = 0;
    temp->enter_time = 0;
//    temp->error = 0;
    memset(temp->id1,0,13);
    memset(temp->id2,0,13);
    temp->leave_time = 0;
    temp->station = 0;
    temp->weight = 0;

}

/****向全局变量的取蛋时间结构体写******/
void Lora_Write_takeegg_buffer(Lora_Buffer temp)
{
    takeegg_buffer.cnt = temp.cnt;
    takeegg_buffer.leave_time = temp.leave_time;
    takeegg_buffer.station = temp.station;
}

/****向全局变量的取蛋时间结构体读******/
void Lora_Read_takeegg_buffer(Lora_Buffer *temp)
{
    temp->cnt = takeegg_buffer.cnt;
    temp->leave_time = takeegg_buffer.leave_time;
    temp->station = takeegg_buffer.station;
}

/****清除数组内容******/
void Lora_Array_clear(void)
{
		int i;
		for(i=0;i<Lora_Arrays.Array_Length;i++)
		{
			Lora_Arrays.Array_date[i].station = Free;
		}
    memset(Lora_Arrays.Array_date,0,Lora_Arrays.Array_Length);
		
		
    Lora_Arrays.Array_Length = 0;
}

/****上电读将数据读到数组，数组写到队列******/
void Lora_Flash_to_queue(void)
{
    int i;
		Lora_Array Lora_Send_Arrays;
    Lora_Array_clear();
	
	  ef_get_env_blob("Lora_Buffer_Send", &Lora_Send_Arrays, sizeof(Lora_Send_Arrays), NULL);//从flash读取到数组		   
    for(i=0;i<Lora_Send_Arrays.Array_Length;i++)
    {
//			Usart1_Printf("Lora_Buffer_Send_length:%d\n", Lora_Arrays.Array_Length);
        if (Lora_Send_Arrays.Array_date[i].station == Sendegg || Lora_Send_Arrays.Array_date[i].station == TakeTime)
        {
            xQueueSend(Lora_Buffer_Send_QueueHandle_t,&(Lora_Send_Arrays.Array_date[i]),0);
        }       
    }
		Usart1_Printf("Lora_Buffer_Send%d\n",uxQueueMessagesWaiting(Lora_Buffer_Send_QueueHandle_t));
    Lora_Array_clear();
	
		ef_get_env_blob("Lora_Buffer_LastSend", &Lora_Arrays, sizeof(Lora_Arrays), NULL);//从flash读取到数组
//    printf("Lora_Buffer_LastSend:%d\n", Lora_Arrays.Array_Length);
    for(i=0;i<Lora_Arrays.Array_Length;i++)
    {
				if (Lora_Arrays.Array_date[i].station == Sendegg || Lora_Arrays.Array_date[i].station == TakeTime)
        {
            xQueueSend(Lora_Buffer_LastSend_QueueHandle_t,&(Lora_Arrays.Array_date[i]),0);
        }		
    }
		Usart1_Printf("Lora_Buffer_LastSend%d\n",uxQueueMessagesWaiting(Lora_Buffer_LastSend_QueueHandle_t));
    Lora_Array_clear();   

    ef_get_env_blob("Lora_Buffer_Store", &Lora_Arrays, sizeof(Lora_Arrays), NULL);//从flash读取到数组
//    printf("Lora_Buffer_Store_length:%d\n", Lora_Arrays.Array_Length);
    for(i=0;i<Lora_Arrays.Array_Length;i++)
    {
        xQueueSend(Lora_Buffer_Store_QueueHandle_t,&(Lora_Arrays.Array_date[i]),0);
    }
		Usart1_Printf("Lora_Buffer_Store%d\n",uxQueueMessagesWaiting(Lora_Buffer_Store_QueueHandle_t));
    Lora_Array_clear();
}

/****数据变动将存进flash******/
void Lora_Buffer_Send_to_Flash(void)
{
	

    Lora_Array_clear();
    while(uxQueueMessagesWaiting(Lora_Buffer_Send_QueueHandle_t) != 0)
    {
        xQueueReceive(Lora_Buffer_Send_QueueHandle_t,&(Lora_Arrays.Array_date[Lora_Arrays.Array_Length]),0);
        Lora_Arrays.Array_Length++;
    }
    for(int i=0;i<Lora_Arrays.Array_Length;i++)
    {
        xQueueSend(Lora_Buffer_Send_QueueHandle_t,&(Lora_Arrays.Array_date[i]),0);
				//Usart1_Printf("Lora_station:%d\n", Lora_Arrays.Array_date[i].station);
    }		
		ef_del_env("Lora_Buffer_Send");
    ef_set_env_blob("Lora_Buffer_Send", &Lora_Arrays, sizeof(Lora_Arrays));
    Lora_Array_clear();
}

void Lora_Buffer_LastSend_to_Flash(void)
{
    Lora_Array_clear();
    while(uxQueueMessagesWaiting(Lora_Buffer_LastSend_QueueHandle_t) != 0)
    {
        xQueueReceive(Lora_Buffer_LastSend_QueueHandle_t,&(Lora_Arrays.Array_date[Lora_Arrays.Array_Length]),0);
        Lora_Arrays.Array_Length++;
    }
    for(int i=0;i<Lora_Arrays.Array_Length;i++)
    {
        xQueueSend(Lora_Buffer_LastSend_QueueHandle_t,&(Lora_Arrays.Array_date[i]),0);
    }
		ef_del_env("Lora_Buffer_LastSend");
    ef_set_env_blob("Lora_Buffer_LastSend", &Lora_Arrays, sizeof(Lora_Arrays));
    Lora_Array_clear();
}
void Lora_Buffer_Store_to_Flash(void)
{
    Lora_Array_clear();
    while(uxQueueMessagesWaiting(Lora_Buffer_Store_QueueHandle_t) != 0)
    {
        xQueueReceive(Lora_Buffer_Store_QueueHandle_t,&(Lora_Arrays.Array_date[Lora_Arrays.Array_Length]),0);
        Lora_Arrays.Array_Length++;
    }
    for(int i=0;i<Lora_Arrays.Array_Length;i++)
    {
        xQueueSend(Lora_Buffer_Store_QueueHandle_t,&(Lora_Arrays.Array_date[i]),0);
    }
		ef_del_env("Lora_Buffer_Store");
    ef_set_env_blob("Lora_Buffer_Store", &Lora_Arrays, sizeof(Lora_Arrays));
    Lora_Array_clear();
}

/*********************************************END OF FILE**********************/
