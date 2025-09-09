#include "./rfid_r200/rfid_r200.h"

u8 RFID_usart_buf[BUF_LENGTH];
struct RFID_ID RFID_ID_buf[CARD_MAX];//卡号存放区,在一次产蛋活动完清空
u8 card_cnt;            //读卡计数,表示一共读到了几张卡，最大值6张,也表示当前卡号的存放下标
u8 read_cnt = 0;        //读卡器串口计数器
u8 empty_id[ID_LENGTH]; //空数组，用于memcmp比较空卡号
u8 rfid_react = 0;      //读卡器响应标志位
u8 id_neglect[ID_LENGTH]; //忽略的ID

extern struct device dev;


//RFID_R200指令
//设置双通道连续盘点
u8 RFID_R200_2D_ConstantRead_cmd[] = {0xBB,0x00,0x1B,0x00,0x03,0x02,0x01,0x01,0x22,0x7E};
//设置四通道连续盘点
u8 RFID_R200_4D_ConstantRead_cmd[] = {0xBB,0x00,0x1B,0x00,0x05,0x02,0x01,0x01,0x01,0x01,0x26,0x7E};
u8 RFID_R200_Read_cmd[] = {0xBB,0x00,0x27,0x00,0x03,0x22,0x00,0x01,0x4D,0x7E};
u8 RFID_R200_4D_OnceRead_cmd[] = {0XBB, 0X00, 0X22, 0X00, 0X00, 0X22, 0X7E};
u8 RFID_HF4channelAntenna1_cmd_init[] = {0xBB,0x00,0x1B,0x00,0x05,0x02,0x01,0x00,0x00,0x00,0x23,0x7E};
u8 RFID_HF4channelAntenna2_cmd_init[] = {0xBB,0x00,0x1B,0x00,0x05,0x02,0x00,0x00,0x01,0x00,0x23,0x7E};
u8 RFID_GetVersion[] = {0XBB,0X00 ,0X03 ,0X00 ,0X01 ,0X00 ,0X04 ,0X7E};
//一体式平板读卡器指令
u8 RFID_Panel_Read_cmd[] = {0xBB, 0x00, 0x22, 0x00, 0x00, 0x22, 0x7E};
u8 RFID_Panel_Get_version_cmd[] = {0XBB, 0X00, 0XC3, 0X00, 0X00, 0XC3, 0X73};

void RFID_SendCmd( USART_TypeDef * pUSARTx, uint8_t *array, uint16_t num)
{
	uint8_t i;

	for(i=0; i<num; i++)
	{
		/* 发送一个字节数据到USART */
		Usart_SendByte(pUSARTx,array[i]);	
	}
	/* 等待发送完成 */
	while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET);
}
/*****************************************************************************
 * @name       :RFID_USART_Config
 * @date       :2024-04-18 16:13:51
 * @function   :初始化R200_2D所用的串口
 * @parameters :无
 * @retvalue   :无
******************************************************************************/
void RFID_USART_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	 NVIC_InitTypeDef NVIC_InitStructure;

	// 打开串口GPIO的时钟
	RFID_USART_GPIO_APBxClkCmd(RFID_USART_GPIO_CLK, ENABLE);
	
	// 打开串口外设的时钟
	RFID_USART_APBxClkCmd(RFID_USART_CLK, ENABLE);

	/* 连接 PXx 到 USARTx_Tx*/
	GPIO_PinAFConfig(RFID_USART_RX_GPIO_PORT,GPIO_PinSource2,GPIO_AF_USART2);

	/*  连接 PXx 到 USARTx__Rx*/
	GPIO_PinAFConfig(RFID_USART_TX_GPIO_PORT,GPIO_PinSource3,GPIO_AF_USART2);
		
	/* 配置Tx引脚为复用功能  */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

	GPIO_InitStructure.GPIO_Pin = RFID_USART_TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RFID_USART_TX_GPIO_PORT, &GPIO_InitStructure);

	/* 配置Rx引脚为复用功能 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = RFID_USART_RX_GPIO_PIN;
	GPIO_Init(RFID_USART_RX_GPIO_PORT, &GPIO_InitStructure);
	
	// 配置串口的工作参数
	// 配置波特率
	USART_InitStructure.USART_BaudRate = RFID_USART_BAUDRATE;
	// 配置 针数据字长
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	// 配置停止位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	// 配置校验位
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	// 配置硬件流控制
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	// 配置工作模式，收发一起
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	// 完成串口的初始化配置
	USART_Init(RFID_USARTx, &USART_InitStructure);
	
	// 串口中断优先级配置
	/* 配置USART为中断源 */
	NVIC_InitStructure.NVIC_IRQChannel = RFID_USART_IRQ;
	/* 抢断优先级*/
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	/* 子优先级 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	/* 使能中断 */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	/* 初始化配置NVIC */
	NVIC_Init(&NVIC_InitStructure);
	
	// 使能串口接收中断
	USART_ITConfig(RFID_USARTx, USART_IT_RXNE, ENABLE);
	USART_ITConfig ( RFID_USARTx, USART_IT_IDLE, ENABLE ); //使能串口总线空闲中断 	
	
	// 使能串口
	USART_Cmd(RFID_USARTx, ENABLE);	      
}
/*****************************************************************************
 * @name       :RFID_Init
 * @date       :2024-04-18 16:14:20
 * @function   :初始化RFID
 * @parameters :无
 * @retvalue   :无
******************************************************************************/
void RFID_Init(void)
{
    #ifdef RFID_R200
        RFID_USART_Config();
        RFID_SendCmd(RFID_USARTx, RFID_HF4channelAntenna1_cmd_init, 12);//选定通道1读卡
        printf("RFID_R200_2D init success\n");
    #else
        RFID_USART_Config();
        printf("RFID_Panel init success\n");
    #endif
}
void RFID_Read(void)
{
    #ifdef RFID_R200
        RFID_SendCmd(RFID_USARTx, RFID_R200_Read_cmd, 10);//寻卡
    #else
        RFID_SendCmd(RFID_USARTx, RFID_Panel_Read_cmd, 7);//寻卡
    #endif
}

void RFID_Heartbreat(void)
{
    static u8 error_cnt;
    RFID_SendCmd(RFID_USARTx, RFID_Panel_Get_version_cmd, 7);
    Delay_ms(10);
    if (rfid_react)
    {
        rfid_react = 0;
        error_cnt = 0;
    }
    else
    {
        //说明无应答
        error_cnt++;
        if (error_cnt >= 10)
        {
            error_cnt = 0;
            //发送错误
            dev.error = ERROR_RFID;
            printf("------------------------------ERROR_RFID\n");
        }
        
    }
}

//进行数据校准并将卡号存入存放区,在出现多卡时会出现帧头0XBB不在第一位的情况，需要进行校准
void RFID_DataProcess(void)
{
    u8 i;
    u8 index;
    u8 exist_flag = 0;//卡号存在标志位，置1表示卡号已存在于存放区中
    /*进行数据校准操作,主要是把帧头移至第一个字节*/
    //只看前5个字节
    for (i = 0; i < 5; i++)
    {
        if (RFID_usart_buf[i] == 0XBB && RFID_usart_buf[i + 1] != 0xBB)
        {
            index = i;//保存最后一个0XBB的下标
            break;
        }
    }
    if (index != 0)
    {
        for (i = 0; i < read_cnt - index; i++)
        {
            RFID_usart_buf[i] = RFID_usart_buf[i + index];
        } 
    }
    if(RFID_usart_buf[0] == 0XBB && RFID_usart_buf[1] == 0x02 && RFID_usart_buf[2] == 0X22)//说明该帧是读到卡的响应帧
    {
//        printf("ID:  ");
//        for(i = 0; i < ID_LENGTH; i++)
//        {
//            printf("%02X ", (RFID_usart_buf + ID_OFFSET)[i]);
//        }
//        printf("\n");
//        
        //如果卡号与id_neglect中的卡号一致，此卡需要被忽略
        if (!memcmp(id_neglect, RFID_usart_buf + ID_OFFSET, ID_LENGTH))
        {
            printf("neglect\n");
            return;
        }
        /*将读到卡号与卡号存放区中已有的卡号进行比对,若匹配到一样的就将exist_flag置为1,并将计数+1*/
        for(i = 0; i < card_cnt; i++)
        {
            if(!memcmp(RFID_ID_buf[i].id, RFID_usart_buf + ID_OFFSET, ID_LENGTH))
            {
//                printf("RFID Crad Exist\n");
                RFID_ID_buf[i].cnt++;
                RFID_ID_buf[i].notcnt = 0;
                exist_flag = 1;
            }
            else
            {
                RFID_ID_buf[i].notcnt++;
            }
        }
        /*若卡号此前不存在，则将卡号拷贝到RFID_ID中*/
        if (!exist_flag)
        {
            if (card_cnt >= CARD_MAX)
            {
                printf("----------------------------RFID_ID_buf is full\n");
            }
            else
            {
//                printf("RFID NEW Card\n");
                memcpy(RFID_ID_buf[card_cnt].id, RFID_usart_buf + ID_OFFSET, ID_LENGTH);
                RFID_ID_buf[card_cnt].cnt++;
                card_cnt++;
            }
        }
        
    }
    else//若没有读到有效卡号,则存放区的所有卡号没被读到的次数+10
    {
        for (i = 0; i < card_cnt; i++)
        {
            if (memcmp(RFID_ID_buf[i].id, empty_id, ID_LENGTH))
            {
                RFID_ID_buf[i].notcnt += 1;//R200在没有读到卡的时候，这个函数执行的周期约2s一次
            }
        }
    }
    /*若某卡号长时间没有继续读到，则将其清理掉*/
	
	
		if(card_cnt != 0)
		{
			for (i = 0; i < card_cnt; i++)  // 只遍历实际使用的卡片数量
			{
				if (RFID_ID_buf[i].notcnt > 20)       // 200次超时清理
				{
					memset(&RFID_ID_buf[i], 0, sizeof(struct RFID_ID));  
					printf("RFID clean\n");
					card_cnt--;  // 正确递减计数器
				}   
			}
		}

		
		//强制清除冗杂缓冲区
		int f = 0;
		for(i = 0; i < CARD_MAX; i++)
		{
			if (!RFID_ID_buf[i].cnt)
			{
				f++;
				if(f ==CARD_MAX)
				card_cnt = 0;	
			}
//			printf("f:%d",f);
		}		

}
//忽略某一ID
void RFID_NeglectID(u8 *id)
{
    u8 i;
    memcpy(id_neglect, id, ID_LENGTH);      //将卡号复制到id_neglect
    //将现有的卡号存放区的此卡号数据清除
    for (i = 0; i < card_cnt; i++)
    {
        if (!memcmp(RFID_ID_buf[i].id, id, ID_LENGTH)) 
        {
            memset(&RFID_ID_buf[i], 0, sizeof(struct RFID_ID));  
            card_cnt--;
        }   
    }
}

void RFID_CleanIDBuf(void)
{
    memset(RFID_ID_buf, 0, sizeof(struct RFID_ID) * CARD_MAX);
    card_cnt = 0;
}
//2通道读卡器串口中断
void RFID_USART_IRQHandler(void)
{
	//判断是接收中断触发
	if(USART_GetITStatus(RFID_USARTx,USART_IT_RXNE))
	{
		//清除中断标志位
		USART_ClearITPendingBit(RFID_USARTx,USART_IT_RXNE);
		//紧急事件
		RFID_usart_buf[read_cnt++] = USART_ReceiveData(RFID_USARTx);

        
	}
	
	//如果要用空闲中断的话要使能串口初始化的空闲中断使能函数
	//判断是空闲中断触发
	if(USART_GetITStatus(RFID_USARTx,USART_IT_IDLE))
	{
		//清除中断标志位
		RFID_USARTx->SR;
		RFID_USARTx->DR;
        rfid_react = 1;
//        printf("DATA:");
//        for (i = 0; i < read_cnt; i++)
//        {
//            printf(" %X", RFID_usart_buf[i]);
//        }
//        printf("\n-------------------------------------\n");
        /*读完卡，进行处理数据以及清空缓存区的操作*/
        RFID_usart_buf[read_cnt] = '\0';
        RFID_DataProcess();
        memset(RFID_usart_buf, 0, BUF_LENGTH);      
        read_cnt = 0;
	}
}
