/*
***********************************************************************************************************




1.将鹅的Flash改为蛋的Flash，并在出窝保存蛋的信息时，添加到Flash。
2.将鹅的产蛋时间修改至微动开关触发。
	添加了FLash校验。
	


***********************************************************************************************************
*/





#include "main.h"
/*舵机变量*/
typedef struct EGG_SIGN
{
    u8 touch;            // 当前触发状态
    TickType_t start_time;
	TickType_t back_time;	// 按下开始时间戳 (关键修改)
    u8 long_press;       // 长按触发标志 (新增)
	
}ES;

ES es={0,0,0,0};

u16 pwmval=0;    
u8 dir=1;
/*舵机变量*/



/*定义任务句柄*/
static TaskHandle_t ReadCardTask_Handle;
static TaskHandle_t	TotalTask_Handle;
static TaskHandle_t IDinandoutTask_Handle;
static TaskHandle_t DatatestTask_Handle;
static TaskHandle_t PWM_START_Task_Handle;
static TaskHandle_t PWM_BACK_Task_Handle;
static TaskHandle_t LoraTask_Handle;
static TaskHandle_t ShowTask_Handle;
/*定义互斥量*/
SemaphoreHandle_t LVGL_mutex;      //LVGL互斥量,用于互斥使用LVGL
SemaphoreHandle_t Usart1_mutex;    //Usart1互斥量,用于互斥使用Usart1
SemaphoreHandle_t Flash_mutex;     //Flash互斥量,用于互斥使用Flash
SemaphoreHandle_t Eggbuf_mutex;    //Eggbuf互斥量,用于互斥使用Eggbuf
SemaphoreHandle_t Usart_lcd_mutex; //串口屏串口互斥量,用于互斥使用串口屏串口
SemaphoreHandle_t Lora_mutex;      //Lora互斥量,用于互斥使用Lora


/*定义蛋信息结构体队列*/
u8 queue_cnt=0;                //队列长度计数器
struct egg queue[MAXLENGTH]; //队列备份,用于恢复
Goose_queue goose_queue ;
u8 goose_queue_cnt=0;
u32 time_nesting = 30; //趴窝的时长定义，方便修改

// 全局变量：记录用户触发的产蛋时间（Unix 时间戳）
uint32_t g_egg_timestamp;

char qr_buf[150]; //二维码信息缓冲区，二维码大小144字节

DEVSTATE totalstate_backup; //状态备份

///*定义lora队列*/
 QueueHandle_t Lora_Buffer_Store_QueueHandle_t;    //数据存储队列
 QueueHandle_t Lora_Buffer_Send_QueueHandle_t;     //数据发送队列
 QueueHandle_t Lora_Buffer_LastSend_QueueHandle_t; //捡蛋时间次数检验队列

/*定义结构体变量*/
struct device dev;
static struct egg egg_temp, egg_empty; //egg_temp是临时蛋信息结构体,egg_empty空蛋信息结构体

Lora_Buffer Lora_Buffer_Temp;
uint32_t last_delate_time;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;  // 避免未使用变量警告  
    Usart1_Printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while(1);  // 死机或重启
}

//队列

int IsEmpty(Goose_queue *Q) {
    // 带头结点的队列：当头结点的 next 为空时队列为空
    // 同时需要确保 head 和 tail 指针已正确初始化（比如初始化后它们指向同一哨兵节点）
    return (Q->head == Q->tail && Q->head->next == NULL);
}

int Init_goose_queue(Goose_queue* Q)//初始化鹅的队列
{
	Q->head=Q->tail=(gooser)malloc(sizeof(goose));
	if(!Q->head)return 0;
	Q->head->next = NULL;
	return 1;	
}

int ADD_goose_queue(Goose_queue *Q)
{
    // 创建新节点
    gooser new_node = (gooser)malloc(sizeof(goose));
    if (!new_node) return 0;  // 内存申请失败
    new_node->next = NULL;
	// 不预先设置ID，让调用者来设置
	new_node->egg_intime = 0;
	new_node->egg_outtime = 0;
    // 将新节点链接到队尾
    Q->tail->next = new_node;  // 哨兵节点的 next 存储第一个有效节点
    Q->tail = new_node;        // 更新尾指针
    return 1;	
}

int De_goose_queue(Goose_queue *Q)
{
    if (Q->tail->next==NULL) {
        return 0;  // 队列空，出队失败
    }
    // 获取待删除节点（哨兵节点的下一个节点）
    gooser del_node = Q->head->next;
    
    // 输出数据（如果参数非 NULL）
    // 更新队列结构
    Q->head->next = del_node->next;  // 哨兵节点链接到下一个节点
    // 如果删除的是最后一个节点，需重置尾指针指向头结点
    if (del_node == Q->tail) 
	{
        Q->tail = Q->head;
    }
    free(del_node);  // 释放内存
    return 1;        // 出队成功
}

void TraverseQueue(Goose_queue *Q, u8 * a) {
    if (IsEmpty(Q)) {
        Usart1_Printf("队列为空\r\n");
        return;
    }
    gooser p = Q->head->next;  // 跳过哨兵节点
    while (p) {
        free(p);       // 处理当前节点
        p = p->next;    // 移动指针到下一节点
    }
}


void De_this_goose_queue(Goose_queue *Q, gooser node)
{
	gooser NODE=Q->head;
    if (IsEmpty(Q))
	{
        Usart1_Printf("队列为空\r\n");
        return;
    }
	
	while(NODE->next != NULL)
	{
		if(NODE->next==node)
		{
			NODE->next=node->next;	
			if(node == Q->tail)
			{
				Q->tail = NODE ;
			}
			break;
		}
		else
		{
			NODE=NODE->next;
		}
	}
}

// ==================== 鹅蛋队列系统 ====================



// 全局鹅蛋队列变量
Egg_queue egg_queue;

// 鹅蛋队列操作函数
int IsEggQueueEmpty(Egg_queue *Q) {
    return (Q->head == Q->tail && Q->head->next == NULL);
}

int Init_egg_queue(Egg_queue* Q) {
    Q->head = Q->tail = (egger)malloc(sizeof(egg_info));
    if(!Q->head) return 0;
    Q->head->next = NULL;
    return 1;
}

int ADD_egg_queue(Egg_queue *Q) {
    egger new_node = (egger)malloc(sizeof(egg_info));
    if (!new_node) return 0;
    new_node->next = NULL;
    // 初始化数据
    memset(new_node->id, 0, LENGTH_ID);
    new_node->egg_intime = 0;
    new_node->egg_outtime = 0;
    new_node->goose_count = 0;
    new_node->egg_timestamp = 0;
    // 链接到队尾
    Q->tail->next = new_node;
    Q->tail = new_node;
    return 1;
}

int De_egg_queue(Egg_queue *Q) {
    if (Q->tail->next == NULL) {
        return 0;  // 队列空，出队失败
    }
    egger del_node = Q->head->next;
    Q->head->next = del_node->next;
    if (del_node == Q->tail) {
        Q->tail = Q->head;
    }
    free(del_node);
    return 1;
}

void De_this_egg_queue(Egg_queue *Q, egger node) {
    egger NODE = Q->head;
    if (IsEggQueueEmpty(Q)) {
        Usart1_Printf("鹅蛋队列为空\r\n");
        return;
    }
    while(NODE->next != NULL) {
        if(NODE->next == node) {
            NODE->next = node->next;
            if(node == Q->tail) {
                Q->tail = NODE;
            }
            free(node);
            break;
        }
        else {
            NODE = NODE->next;
        }
    }
}

//队列

// 判断卡号是否已在队列中
int IsGooseInQueue(Goose_queue *Q, u8 *id) {
    gooser node = Q->head->next;
    while(node != NULL) {
        if(memcmp(node->id, id, ID_LENGTH) == 0) {
            return 1; // 已存在
        }
        node = node->next;
    }
    return 0; // 不存在
}


static void ReadCardTask(void *param)
{
    /*读卡任务，循环发送读卡指令*/
    u8 i;
    u8 flag_in = 0;
    u8 cnt = 0;
    Usart1_Printf("ReadCardTask Create Success\r\n");
	Usart1_Printf("TASK 1!\r\n");
    while (1)
    {
        RFID_Read();
        cnt++;
        if (cnt >= 10)
        {
            cnt = 0;
            RFID_Heartbreat(); //心跳
        }
        for (i = 0; i < card_cnt; i++)
        {
            if (memcmp(RFID_ID_buf[i].id, empty_id, ID_LENGTH)) //是则说明id非空，说明存卡区有卡
            {
                if (RFID_ID_buf[i].notcnt < 10) //说明在鹅窝中
                {
                    dev.flag_goosein = 1; //有鹅在鹅窝标志位
                    flag_in = 1;
                    break;
                }
            }
        }
        if (!flag_in)
        {
            dev.flag_goosein = 0;
        }
        flag_in = 0;
        vTaskDelay(pdMS_TO_TICKS(300)); //延时100ms
    }
}

static void TotalTask(void *param)
{
    gooser node;
    u8 i;
    u8 still_in = 0;
    u32 timeout_cnt = 0;
    u8 id1_cnt = 0;   //保存id1的下标
    u8 id2_cnt = 0;   //保存id2的下标
    u8 id_cnt = 0;   //保存id1的下标
    u8 flag_nest = 1; //趴窝标志位，一次只发一次
    u8 flag_show = 1;
    Usart1_Printf("TotalTask Create Success\r\n");
    Usart1_Printf("TASK 2!\r\n");
    while(1)
    {
        vTaskDelay(1000);
        flag_nest=1;
        for (i = 0; i < CARD_MAX; i++)
        {
            if (memcmp(RFID_ID_buf[i].id, empty_id, ID_LENGTH)) //是则说明非空，说明存卡区有卡
            {
                if (RFID_ID_buf[i].cnt > 10) //连续读到大于100次,判定为入窝，可调整
                {
                    if(dev.flag_haveread)
                    {
                        if(!IsGooseInQueue(&goose_queue, RFID_ID_buf[i].id))
                        {
                            ADD_goose_queue(&goose_queue);
                            id2_cnt = i;
                            memcpy(egg_temp.id1, RFID_ID_buf[i].id, ID_LENGTH); //将这个ID拷贝到egg_temp结构体中
                            memcpy(goose_queue.tail->id, RFID_ID_buf[i].id, ID_LENGTH); //将ID信息入队                        
                            goose_queue.tail->egg_intime = MyRTC_GetUnixTimestamp(); //保存入窝时间    
                            egg_temp.egg_intime = MyRTC_GetUnixTimestamp(); //保存入窝时间							
                            break;
                        }
                                
                    }
                        
										
										
                    else                            
                    {    
                        if(!IsGooseInQueue(&goose_queue, RFID_ID_buf[i].id))
                        {
                            ADD_goose_queue(&goose_queue);//入队
                            id1_cnt = i;                            
                            memcpy(goose_queue.tail->id, RFID_ID_buf[i].id, ID_LENGTH); //将ID信息入队                        
                            goose_queue.tail->egg_intime = MyRTC_GetUnixTimestamp(); //保存入窝时间                            
                            Usart1_Printf("ID1 READ SUCCESS ,intime save success\r\n");
                            dev.flag_haveread=1;
                        }
                    }
                }
                    
                if(dev.flag_haveread && RFID_ID_buf[id1_cnt].notcnt > 500)
                {    
                    dev.flag_haveread=0;                        
                }        
            }
        }    
    }
}



static void IDinandoutTask(void *param)
{
	gooser node;
	u8 i;
	u8 still_in = 0;
	u8 goose_count = 0;  // 计算鹅的数量
	Usart1_Printf("TASK 3!\r\n");
	while(1)
	{
		vTaskDelay(1000);
		Usart1_Printf("card_cnt:%d\r\n",card_cnt);			
		if( !IsEmpty(&goose_queue) || card_cnt)
		{				
			// 计算当前鹅的数量
			goose_count = 0;
			node = goose_queue.head->next;
			while(node != NULL) {
				goose_count++;
				node = node->next;
			}
			
			//100 * 60ms = 6s 6s没被读到，判断为走出鹅窝
			for(i = 0; i < CARD_MAX; i++)
			{
				if (RFID_ID_buf[i].notcnt >= 10) //当卡号没被读到的次数大于100次，判定为鹅走出鹅窝---------------------------这里方便调试给他调小，注意更改
				{    
					// 在队列中找到对应的鹅
					node = goose_queue.head->next;
					while(node != NULL) 
					{
						if(memcmp(RFID_ID_buf[i].id, node->id, ID_LENGTH) == 0) 
						{
//							// 先找到对应的鹅，再检查是否需要记录出窝时间
//							if(dev.flag_egg_ready == 1) 
//							{
//								// 如果是序号为1的鹅（队头）
//								if(node == goose_queue.head->next)
//								{			
//									node->egg_outtime = MyRTC_GetUnixTimestamp(); // 记录出窝时间									
//									// 创建鹅蛋数据并保存
//									if(ADD_egg_queue(&egg_queue)) 
//									{
//										memcpy(egg_queue.tail->id, node->id, ID_LENGTH);
//										egg_queue.tail->egg_intime = node->egg_intime;
//										egg_queue.tail->egg_outtime = node->egg_outtime;
//										egg_queue.tail->goose_count = goose_count;
//										egg_queue.tail->egg_timestamp = g_egg_timestamp;
//										
//										// 将出窝时间保存到flash
//										Egg_Queue Flash_egg;
//										memcpy(Flash_egg.id, node->id, ID_LENGTH);
//										Flash_egg.egg_intime = node->egg_intime;
//										Flash_egg.egg_outtime = node->egg_outtime;
//										Flash_egg.goose_count = goose_count;
//										Flash_egg.egg_timestamp = g_egg_timestamp;//产蛋时间
//										Egg_Queue_Add(&Flash_egg);
//										Usart1_Printf("Goose outtime updated and saved to Flash\r\n");
//										
//										Usart1_Printf("Egg data saved");
//									}									
//									dev.flag_egg_ready = 0;  // 重置标志位
//								}
//							}
//						不可以滚多蛋版本

							// 先找到对应的鹅，再检查是否需要记录出窝时间
							if(node == goose_queue.head->next)			
							{
								// 如果是序号为1的鹅（队头）
								while(dev.flag_egg_ready) 
								{			
									node->egg_outtime = MyRTC_GetUnixTimestamp(); // 记录出窝时间									
									// 创建鹅蛋数据并保存
									if(ADD_egg_queue(&egg_queue)) 
									{
										memcpy(egg_queue.tail->id, node->id, ID_LENGTH);
										egg_queue.tail->egg_intime = node->egg_intime;
										egg_queue.tail->egg_outtime = node->egg_outtime;
										egg_queue.tail->goose_count = goose_count;
										egg_queue.tail->egg_timestamp = g_egg_timestamp;
										
										// 将出窝时间保存到flash
										Egg_Queue Flash_egg;
										memcpy(Flash_egg.id, node->id, ID_LENGTH);
										Flash_egg.egg_intime = node->egg_intime;
										Flash_egg.egg_outtime = node->egg_outtime;
										Flash_egg.goose_count = goose_count;
										Flash_egg.egg_timestamp = g_egg_timestamp;//产蛋时间
										Egg_Queue_Add(&Flash_egg);
										Usart1_Printf("Goose outtime updated and saved to Flash\r\n");			
										Usart1_Printf("Egg data saved");
									}									
									dev.flag_egg_ready --;  // 重置标志位
									vTaskDelay(1000);
								}
							}
							break;  // 找到鹅后退出循环
						}
						node = node->next;
					}
					
					memset(&RFID_ID_buf[i], 0, sizeof(struct RFID_ID));  					
					Usart1_Printf("Goose OUT\r\n");
				}				
			}					
			//若是缓冲区中已经被清除的鹅还在队列中，则让其出队        
			node = goose_queue.head->next;    
			while(node != NULL)
			{
				still_in = 0;  // 重置标志位
				for(i = 0; i < CARD_MAX; i++)
				{
					// 修复：检查是否相等而不是不相等
					if (memcmp(RFID_ID_buf[i].id, node->id, ID_LENGTH) == 0)
					{
						still_in = 1;
						break;  // 找到匹配的就退出循环
					}
				}
						
				if(still_in == 0)
				{
					// 鹅已经不在缓冲区中，需要出队
					gooser next_node = node->next;  // 保存下一个节点
					De_this_goose_queue(&goose_queue, node);
					node = next_node;  // 移动到下一个节点
				}
				else
				{
					node = node->next;  // 移动到下一个节点
				}
			}			
		}			
	}
}



static void DatatestTask(void *param)
{
	u8 i=1;
	while(1)
	{
		vTaskDelay(1000);
		Usart1_Printf("TASK4!\r\n");
//		Usart1_Printf("\r\n");
		// 打印鹅队列
		if(!IsEmpty( &goose_queue))
		{
			Usart1_Printf("=== GOOSE QUEUE ===\r\n");
			gooser node = goose_queue.head->next;
			i = 1;
			while(node != NULL)
			{
				Usart1_Printf("Goose %d: ID=", i);
				for(int k=0; k<ID_LENGTH; k++) {
					Usart1_Printf("%02X", node->id[k]);
				}
				Usart1_Printf(", intime=%lu, outtime=%lu\r\n", node->egg_intime, node->egg_outtime);
				node=node->next;
				i++;
			}
		}
		
		// 打印鹅蛋队列
		if(!IsEggQueueEmpty(&egg_queue))
		{
			Usart1_Printf("=== EGG QUEUE ===\r\n");
			egger egg_node = egg_queue.head->next;
			i = 1;
			while(egg_node != NULL)
			{
				Usart1_Printf("Egg %d: ID=", i);
				for(int k=0; k<ID_LENGTH; k++) {
					Usart1_Printf("%02X", egg_node->id[k]);
				}
				Usart1_Printf(", intime=%lu, outtime=%lu, count=%d, timestamp=%lu\r\n", egg_node->egg_intime, 
				egg_node->egg_outtime,egg_node->goose_count, egg_node->egg_timestamp);
				
//				Usart1_Printf("Egg %d: ID=%02X..., intime=%lu, outtime=%lu, count=%d, timestamp=%lu\r\n", 
//							  i, egg_node->id[0], egg_node->egg_intime, egg_node->egg_outtime, 
//							  egg_node->goose_count, egg_node->egg_timestamp);
				egg_node=egg_node->next;
				i++;
			}
		}
	}
}



void PWM_START_Task(void *param)
{
	
	TickType_t current_time;
	Usart1_Printf("TASK 5!\r\n");
	const TickType_t hold_duration = pdMS_TO_TICKS(5000); // 5秒阈值
	while(1)
	{	
		vTaskDelay(1000);
		
		
		if(WK_UP)
		{
			vTaskDelay(pdMS_TO_TICKS(50));
			if(es.touch==0)
			{
			es.touch=1;	
			es.start_time = xTaskGetTickCount();
			Usart1_Printf("BUTTON has been pressed , start counting\r\n");				
			}			
		}
		
		else 
		{
		es.touch=0;	
			
		}
		
		if(es.touch==1)
		{
			current_time = xTaskGetTickCount();
			
			if((current_time - es.start_time) >= hold_duration)
			{							
				if(es.long_press == 0) 
				{  // 首次满足10秒条件
                    es.long_press = 1;
                    dev.flag_egg_ready ++;  // 设置蛋准备进入标志位
										g_egg_timestamp = MyRTC_GetUnixTimestamp();  
                    Usart1_Printf("10 seconds reached, egg ready to enter\r\n");									
				}			
			}			
		}
		
		if(es.long_press == 1)
		{
			
			TIM_SetCompare1(TIM4,1500);
			Usart1_Printf("90。\r\n");
			
		}

	}
	
	
}


void PWM_BACK_Task(void *param)
{
	
	TickType_t current2_time;
	const TickType_t back_duration = pdMS_TO_TICKS(5000); // 5秒阈值
	Usart1_Printf("TASK 6!\r\n");
	while(1)
	{
		vTaskDelay(1000);
		if(!WK_UP)
		{
			vTaskDelay(pdMS_TO_TICKS(50));
			if(es.touch==1)
			{
			es.touch=0;	
			es.back_time = xTaskGetTickCount();
			Usart1_Printf("BUTTON has not been pressed, start counting.\r\n");				
			}			
		}
				
		if(es.touch==0)
		{
			current2_time = xTaskGetTickCount();
			if((current2_time - es.back_time) >= back_duration)
			{								
				if(es.long_press == 1) 
				{  // 首次满足2秒条件
                    es.long_press = 0;
                    Usart1_Printf("WAITING for 5 seconds! return:0\r\n");									
				}			
			}			
		}
	
		if(es.long_press==0)
		{
			
			TIM_SetCompare1(TIM4,500);
			Usart1_Printf("\n0du \r\n");				
		}
	}	
}






static void ShowTask(void *param)
{
    /*该任务是取蛋任务*/
    Usart1_Printf("ShowTask Create Success\n");
    u32 timeout_cnt; //超时计数器
    static u8 current_egg_index = 0; // 当前显示的蛋索引
    static u8 total_eggs = 0; // 总蛋数
    static u8 qr_display_mode = 0; // 二维码显示模式：0-未显示，1-显示中
    while (1)
    {
        Usart_lcd_updatetime(); //更新时间
        Usart_lcd_heartbeat();  //心跳
        
        // 计算蛋队列中的蛋数量
        if(!IsEggQueueEmpty(&egg_queue))
        {
            egger temp_node = egg_queue.head->next;
            total_eggs = 0;
            while(temp_node != NULL)
            {
                total_eggs++;
                temp_node = temp_node->next;
            }
        }
        else
        {
            total_eggs = 0;
            current_egg_index = 0;
            qr_display_mode = 0;
        }
        
        // 如果取蛋按钮被短按，进入二维码显示模式或切换到下一个二维码
        if (TakeEgg_Key.start_key_state == KEY_SHORTPRESS)
        {
            TakeEgg_Key.start_key_state = KEY_NOTPRESSED;
            
            if(total_eggs > 0)
            {
                if (qr_display_mode != 1)
                {
                    qr_display_mode = 1; // 进入二维码显示模式
                    current_egg_index = 0; // 从第一个蛋开始显示
                    Usart1_Printf("进入二维码显示模式\r\n");
                    
                    // 生成lora包，向服务器查询捡蛋结果
                    memset(&Lora_Buffer_Temp, 0, sizeof(Lora_Buffer_Temp));
                    Lora_Buffer_Temp.station = TakeTime;
                    Lora_Buffer_Temp.cnt = 0;
                    xQueueSend(Lora_Buffer_Send_QueueHandle_t, &Lora_Buffer_Temp, 0); //入队
                }
                else
                {
                    current_egg_index = (current_egg_index + 1) % total_eggs; // 循环切换到下一个蛋
                    Usart1_Printf("切换到第%d个蛋的二维码，共%d个蛋\r\n", current_egg_index + 1, total_eggs);
                }
                
                // 显示当前索引对应的蛋的二维码
                memset(qr_buf, 0, 150); // 先将qr_buf清空
                
                // 找到第current_egg_index个蛋
                egger current_egg = egg_queue.head->next;
                u8 temp_index = 0;
                while(current_egg != NULL && temp_index < current_egg_index)
                {
                    current_egg = current_egg->next;
                    temp_index++;
                }
                
                if(current_egg != NULL)
                {
                    base64_convertegg(current_egg, 1);
                    base64_encode(qr_buf);
                    Usart1_Printf("qr:%s\n", qr_buf);
                    Usart_lcd_qrcode_update(qr_buf);
                    Usart_lcd_qrcode_show_info(current_egg_index, total_eggs);
                }
            }
						
//						//刚上电时，将Flash的蛋队列显示二维码
//						else if(Egg_Queue_Flag)
//						{
//							//Flash队列
//							if (!egg_queue_array.Egg_queue_length)
//							{
//									qr_display_mode = 1; // 进入二维码显示模式
//									current_egg_index = 0; // 从第一个蛋开始显示
//									Usart1_Printf("进入 FLash 二维码显示模式\r\n");
//									
//									// 生成lora包，向服务器查询捡蛋结果
//									memset(&Lora_Buffer_Temp, 0, sizeof(Lora_Buffer_Temp));
//									Lora_Buffer_Temp.station = TakeTime;
//									Lora_Buffer_Temp.cnt = 0;
//									xQueueSend(Lora_Buffer_Send_QueueHandle_t, &Lora_Buffer_Temp, 0); //入队
//							}
//							else
//							{
//									current_egg_index = (current_egg_index + 1) % total_eggs; // 循环切换到下一个蛋
//									Usart1_Printf("Flash 切换到第%d个蛋的二维码，共%d个蛋\r\n", current_egg_index + 1, total_eggs);
//							}
//							
//							// 显示当前索引对应的蛋的二维码
//							memset(qr_buf, 0, 150); // 先将qr_buf清空
//							
//							// 找到第current_egg_index个蛋
//							egger current_egg = egg_queue.head->next;
//							u8 temp_index = 0;
//							while(current_egg != NULL && temp_index < current_egg_index)
//							{
//									current_egg = current_egg->next;
//									temp_index++;
//							}
//							
//							if(current_egg != NULL)
//							{
//									base64_convertegg(current_egg, 1);
//									base64_encode(qr_buf);
//									Usart1_Printf("qr:%s\n", qr_buf);
//									Usart_lcd_qrcode_update(qr_buf);
//									Usart_lcd_qrcode_show_info(current_egg_index, total_eggs);
//							}
//								
//						}
//						
						
            else
            {
                Usart1_Printf("没有蛋信息可显示\r\n");
            }
        }
				
				
				//长按退出二维码，并清空内存队列和Flash的队列
				if(Egg_Queue_Flag && TakeEgg_Key.start_key_state == KEY_LONGPRESS1)
				{
				TakeEgg_Key.start_key_state = KEY_NOTPRESSED;
				Egg_Queue_Flag = 0;
				
								// 添加验证信息
//				Usart1_Printf("验证清除结果: egg_queue_array.Egg_queue_length = %d\r\n", 
//							  egg_queue_array.Egg_queue_length);
					
					
				Usart1_Printf("已清除内存队列和Flash队列的信息");
				Usart_lcd_noegg();
				//清除Flash中的历史数据
				Egg_Queue_Reset();
				Usart1_Printf("Egg queue historical data cleared\r\n");
				//清除内存的蛋的队列信息
				Usart1_Printf("Egg queue historical data cleared\r\n");
    
				// 添加验证信息
				Usart1_Printf("验证清除结果: egg_queue_array.Egg_queue_length = %d\r\n", 
							  egg_queue_array.Egg_queue_length);
				
				egger current = egg_queue.head->next;
				while(current != NULL) {
					egger next = current->next;
					free(current);
					current = next;
				}
				egg_queue.head->next = NULL;
				egg_queue.tail = egg_queue.head;


				}
			

				
        vTaskDelay(pdMS_TO_TICKS(1000)); //延时100ms
//		Usart1_Printf("Egg_Queue_Flag:%d\r\n",Egg_Queue_Flag);		
    }
}


static void LoraTask(void *param)
{
    static uint32_t u32_wtimestamp = 3;//趴窝时间用于长时间霸占鹅窝的警报，单位是小时
    static struct tm *enter_time;//鹅进入窝的时间
    static struct tm *leave_time;//鹅离开窝的时间
		BitAction send_review_flag = 0; //是否要读服务器返回的数据标志位
    struct tm LoraTask_Nowtime;     //当前时间
    struct tm LoraTask_Livetime;    //上一次发送心跳时间
		int LoraTask_Lasttime;          //上一次唤醒的分钟，让一分钟内不会连续唤醒，因为LG210的bug
    u8 LoraTask_GetTime_flag = 1, 
			 LoraTask_Sendegg_flag = 1, 
			 LoraTask_TakeTime_flag = 1, 
			 LoraTask_Live_flag = 0; //防止心跳在一分钟内多次发送
    Lora_Buffer temp;               //用于以下队列的缓存
    /*该任务是专门进行Lora通讯的*/
    Usart1_Printf("LoraTask Create Success\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    Lora_Buffer last_temp;//提取Lora_Buffer_LastSend_QueueHandle_t然后用于缓存
    Lora_Buffer send_temp;//提取Lora_Buffer_Send_QueueHandle_t然后用于缓存

    xQueuePeek(Lora_Buffer_LastSend_QueueHandle_t, &(last_temp), 0);//偷看Lora_Buffer_LastSend_QueueHandle_t存放到last_temp
    xQueuePeek(Lora_Buffer_Send_QueueHandle_t, &(send_temp), 0);//偷看Lora_Buffer_Send_QueueHandle_t存放到send_temp

		/*****上电初始化先校验系统时间*****/
    temp.station = GetTime;
    temp.cnt = 0;
    xQueueSendToFront(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
    Lora_CLear_Lora_buffer(&temp);

		/*****上电读取网关缓存*****/
    temp.station = Live;
    xQueueSendToFront(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
    Lora_CLear_Lora_buffer(&temp);
    while (1)
    {
				/*****30分钟发一次心跳，用于服务器确认装置是否断开*****/
        LoraTask_Nowtime = MyRTC_Lora_GetTime();
        if ((LoraTask_Nowtime.tm_min == 15 || LoraTask_Nowtime.tm_min == 45) && LoraTask_Live_flag == 1)
        {
            //Usart1_Printf("nowt:%d,lastt:%d", LoraTask_Nowtime.tm_min, LoraTask_Livetime.tm_min);
            LoraTask_Live_flag = 0;
            //Usart1_Printf("test30%d,%d\n",LoraTask_Nowtime.tm_hour,LoraTask_Nowtime.tm_min);
            temp.station = Live;
            xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
            Lora_CLear_Lora_buffer(&temp);
        }
        else if ((LoraTask_Nowtime.tm_min == 16 || LoraTask_Nowtime.tm_min == 46) && LoraTask_Live_flag == 0)
        {
            LoraTask_Live_flag = 1;
        }
        /*****每天14点校验时间*****/
        if (LoraTask_Nowtime.tm_hour == 14 && LoraTask_GetTime_flag == 1)
        {
            //Usart1_Printf("nowt:%d,\n", LoraTask_Nowtime.tm_hour);
            temp.station = GetTime;
            temp.cnt = 0;
            xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
            Lora_CLear_Lora_buffer(&temp);
            LoraTask_GetTime_flag = 0;
        }
        else if (LoraTask_Nowtime.tm_hour == 15 && LoraTask_GetTime_flag == 0)//用于第二天的14点再次发送
        {
            LoraTask_GetTime_flag = 1;
        }

        /******每天15点检查未发送的产蛋信息*****/
        if (LoraTask_Nowtime.tm_hour == 3 && LoraTask_Sendegg_flag == 1 && uxQueueMessagesWaiting(Lora_Buffer_Store_QueueHandle_t) != 0)
        {

            while (uxQueueMessagesWaiting(Lora_Buffer_Store_QueueHandle_t))
            {
                //Usart1_Printf("nowt:%d,\n", LoraTask_Nowtime.tm_hour);
                xQueueReceive(Lora_Buffer_Store_QueueHandle_t, &temp, 0);
                xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                //                temp.station = Sendegg;
                //                xQueueSend(Lora_Buffer_Send_QueueHandle_t,&temp,0);
                Lora_CLear_Lora_buffer(&temp);
            }
            LoraTask_Sendegg_flag = 0;
        }
        else if (LoraTask_Nowtime.tm_hour == 4 && LoraTask_Sendegg_flag == 0)//用于第二天的16点再次发送
        {
            LoraTask_Sendegg_flag = 1;
        }
        /******每天16点检查捡蛋信息*****/
        if (LoraTask_Nowtime.tm_hour == 4 && LoraTask_TakeTime_flag == 1)
        {
            Usart1_Printf("nowt:%d,\n", LoraTask_Nowtime.tm_hour);
            temp.station = TakeTime;
            xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
            Lora_CLear_Lora_buffer(&temp);
            LoraTask_TakeTime_flag = 0;
        }
        else if (LoraTask_Nowtime.tm_hour == 5 && LoraTask_TakeTime_flag == 0)//用于第二天的17点再次发送
        {
            LoraTask_TakeTime_flag = 1;
        }
        if (Lora_Flag == 1)//被轮循到
        {
					if (LoraTask_Nowtime.tm_min != LoraTask_Lasttime)//不是在同一分钟内被轮询两次进入
            {
                LoraTask_Lasttime = LoraTask_Nowtime.tm_min;//存储最近一次被轮询进入后的时间
								if (uxQueueMessagesWaiting(Lora_Buffer_LastSend_QueueHandle_t) != 0)//如果存在未接收的数据
                {
                    send_review_flag = 1;
                }
                if (xQueueReceive(Lora_Buffer_Send_QueueHandle_t, &(temp), 0) == pdPASS)//发送队列有东西
                {
                    /**************************************根据状态发送信息给服务器***********************************/
                    switch (temp.station)
                    {
											//发送校验时间
											case (GetTime):
											{
													Lora_printf("{\"type\":\"1\",\"data\":{},\"nid\":\"%s\"}", Lora_NID); //发送获取时间指令
													xQueueSend(Lora_Buffer_LastSend_QueueHandle_t, &temp, 0);
													//Usart1_Printf("temp%d",temp.station);
												
													Lora_CLear_Lora_buffer(&temp);
													temp.station = Live;
													xQueueSendToFront(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
													break;
											}
											//发送产蛋信息
											case (Sendegg):
											{
													enter_time = localtime(&(temp.enter_time)); //转换成星期，月，日，时分秒，年的字符串
													char enter_time_buffer[80];
													strftime(enter_time_buffer, sizeof(enter_time_buffer), "%Y-%m-%d %H:%M", enter_time);//转换后端需要的格式

													leave_time = localtime(&(temp.leave_time)); //转换成星期，月，日，时分秒，年的字符串
													char leave_time_buffer[80];
													strftime(leave_time_buffer, sizeof(leave_time_buffer), "%Y-%m-%d %H:%M", leave_time);//转换后端需要的格式

													Lora_printf("{\"type\":\"2\",\"data\":{\"lrfid\":\"%02X%02X%02X%02X\",\"rrfid\":\"%02X%02X%02X%02X\",\"weight\":\"%d\",\"entertime\":\"%s\",\"leavetime\":\"%s\"},\"nid\":\"%s\"}\n", temp.id1[8], temp.id1[9], temp.id1[10], temp.id1[11], temp.id2[8], temp.id2[9], temp.id2[10], temp.id2[11], temp.weight, enter_time_buffer, leave_time_buffer, Lora_NID);
													xQueueSend(Lora_Buffer_LastSend_QueueHandle_t, &temp, 0);

													Lora_CLear_Lora_buffer(&temp);
													temp.station = Live;
													xQueueSendToFront(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
													break;
											}
											//发送错误信息
											case (Error):
											{
													leave_time = localtime(&(temp.leave_time));//转换成星期，月，日，时分秒，年的字符串
													char leave_time_buffer[80];
													strftime(leave_time_buffer, sizeof(leave_time_buffer), "%Y-%m-%d %H:%M", leave_time); //转换后端需要的格式
												
//													if (temp.error == ERROR_NEST)//有些错误信息需要携带脚环
//													{
//															Lora_printf("{\"type\":\"3\",\"data\":{\"extype\":%d,\"Irfid\":\"%02x%02x%02x%02x\",\"rrfid\":\"%02x%02x%02x%02x\",\"happentime\":\"%s\",\"hashcode\":\"null\"},\"nid\":\"%s\"}", //发送错误
//																					temp.error, temp.id1[8], temp.id1[9], temp.id1[10], temp.id1[11], temp.id2[8], temp.id2[9], temp.id2[10], temp.id2[11], leave_time_buffer, Lora_NID);
//													}
//													else
//													{
//															Lora_printf("{\"type\":\"3\",\"data\":{\"extype\":%d,\"happentime\":\"%s\",\"hashcode\":\"null\"},\"nid\":\"%s\"}", //发送错误
//																					temp.error, leave_time_buffer, Lora_NID);
//													}
													xQueueSend(Lora_Buffer_LastSend_QueueHandle_t, &temp, 0);

													//测试完删除,为了快点看到产蛋信息
													Lora_CLear_Lora_buffer(&temp);
													temp.station = Live;
													xQueueSendToFront(Lora_Buffer_Send_QueueHandle_t, &temp, 0);

													break;
											}
											//发送心跳信息
											case (Live):
											{

													LoraTask_Livetime = MyRTC_GetTime();
													Lora_printf("{\"type\":\"4\",\"data\":{\"wtime\":\"%d\"},\"nid\":\"%s\"}", u32_wtimestamp, Lora_NID);
													LoraTask_Live_flag = 1;

													break;
											}
											//发送询问取蛋时间
											case (TakeTime):
											{

													Lora_printf("{\"type\":\"5\",\"data\":{},\"nid\":\"%s\"}", Lora_NID);
													xQueueSend(Lora_Buffer_LastSend_QueueHandle_t, &temp, 0);

												if (uxQueueMessagesWaiting(Lora_Buffer_Send_QueueHandle_t) == 0)//如果队列为空，下一次轮询发送心跳用于校验
													{
															Lora_CLear_Lora_buffer(&temp);
															temp.station = Live;
															xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
													}
													break;
											}
											default:
											{
													//printf("default\n");
													break;
											}
                    }
                }
                Lora_CLear_Lora_buffer(&temp); //发送完清除标志位

                /**************************************接收服务器的数据***********************************/
                vTaskDelay(pdMS_TO_TICKS(1000));
                //Usart1_Printf("r:%s\n", Lora_Struct.USART_BUFF);
                xQueuePeek(Lora_Buffer_LastSend_QueueHandle_t, &temp, 0);
                //Usart1_Printf("test_xQueuePeek:%d\n",temp.station);

                /********接收修改趴窝时间********/
                if (strstr((char *)Lora_Struct.USART_BUFF, "systime") && temp.station != GetTime) //确认格式，提取时间戳
                {
                    char *timestamp_start = strstr(Lora_Struct.USART_BUFF, "\"systime\":\"") + 11;
                    char *timestamp_end = strchr(timestamp_start, '\"');
                    size_t timestamp_length = timestamp_end - timestamp_start;
                    char timestamp[timestamp_length + 1];
                    strncpy(timestamp, timestamp_start, timestamp_length);
                    timestamp[timestamp_length] = '\0';
                    uint32_t u32_timestamp = atoi(timestamp); //提取后时间戳存放在u32_timestamp

                    char *wtimestamp_start = strstr(Lora_Struct.USART_BUFF, ",\"wtime\":") + 9;
                    char *wtimestamp_end = strchr(wtimestamp_start, '}');
                    size_t wtimestamp_length = wtimestamp_end - wtimestamp_start;
                    char wtimestamp[wtimestamp_length + 1];
                    strncpy(wtimestamp, wtimestamp_start, wtimestamp_length);
                    wtimestamp[wtimestamp_length] = '\0';
                    u32_wtimestamp = atoi(wtimestamp);
                    time_nesting = atoi(wtimestamp) * 3600; //提取后时间戳存放在time_nesting
                    
                    ef_set_int("time_nesting", time_nesting);//将趴窝时间存入flash中

//                    Usart1_Printf("nesting:%d", u32_wtimestamp);
                }
                Lora_CLear_Lora_buffer(&temp);//清除缓冲区

                if (send_review_flag == 1)//存在需要接收的数据
                {
                    send_review_flag = 0;
                    if (xQueueReceive(Lora_Buffer_LastSend_QueueHandle_t, &(temp), 0) == pdPASS)//提取需要接收的类型
                    {
                        //Usart1_Printf("last%d\n", temp.station);
                        switch (temp.station)
                        {
													//接收时间
                        case (GetTime):
                        {
                            if (Lora_Struct.USART_Length != 0 && strstr((char *)Lora_Struct.USART_BUFF, "systime"))//查询到关键数据
                            {
                                Lora_Struct.USART_BUFF[Lora_Struct.USART_Length] = '\0';//添加字符串结束帧防止出错
																
																//这一段是解析数据的
                                char *timestamp_start = strstr(Lora_Struct.USART_BUFF, "\"systime\":\"") + 11;
                                char *timestamp_end = strchr(timestamp_start, '\"');
                                size_t timestamp_length = timestamp_end - timestamp_start;
                                char timestamp[timestamp_length + 1];
                                strncpy(timestamp, timestamp_start, timestamp_length);
                                timestamp[timestamp_length] = '\0';
                                uint32_t u32_timestamp = atoi(timestamp) + 60; //提取后时间戳存放在u32_timestamp

                                char *wtimestamp_start = strstr(Lora_Struct.USART_BUFF, ",\"wtime\":") + 9;
                                char *wtimestamp_end = strchr(wtimestamp_start, '}');
                                size_t wtimestamp_length = wtimestamp_end - wtimestamp_start;
                                char wtimestamp[wtimestamp_length + 1];
                                strncpy(wtimestamp, wtimestamp_start, wtimestamp_length);
                                wtimestamp[wtimestamp_length] = '\0';
                                u32_wtimestamp = atoi(wtimestamp);
                                time_nesting = atoi(wtimestamp) * 3600; //提取后时间戳存放在wtimestamp
                                ef_set_int("time_nesting", time_nesting);
                                
                                struct tm *LoraTask_Temp = localtime(&u32_timestamp); //时间戳转结构体
                                Timestamp_Set_RTC_Time(u32_timestamp);//将时间更新到RTC中

                                //Usart1_Printf("time%d,%d\n", u32_timestamp, time_nesting);
                            }
                            else
                            {
                                temp.cnt++;//错误次数+1
                                //Usart1_Printf("notime:%d\n", temp.cnt);
                                if (temp.cnt <= 2)
                                {
                                    xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                                }
                                Lora_CLear_Lora_buffer(&temp);
                            }
                            break;
                        }
												//校验蛋信息
                        case (Sendegg):
                        {
                            //Usart1_Printf("test_LastSend_Sendegg");
                            if (Lora_Struct.USART_Length != 0 && strstr((char *)Lora_Struct.USART_BUFF, "lrfid"))
                            {
                                /*提取后lrfid的数据存放在lrfidtemp*/
                                char *lrfidtemp_start = strstr(Lora_Struct.USART_BUFF, "\"lrfid\":\"") + 9;
                                char *lrfidtemp_end = strchr(lrfidtemp_start, '\"');
                                size_t lrfidtemp_length = lrfidtemp_end - lrfidtemp_start;
                                char lrfidtemp[lrfidtemp_length + 1];
                                strncpy(lrfidtemp, lrfidtemp_start, lrfidtemp_length);
                                lrfidtemp[lrfidtemp_length] = '\0';

                                /*提取后rrfid的数据存放在rrfidtemp*/
                                char *rrfidtemp_start = strstr(Lora_Struct.USART_BUFF, "\"rrfid\":\"") + 9;
                                char *rrfidtemp_end = strchr(rrfidtemp_start, '\"');
                                size_t rrfidtemp_length = rrfidtemp_end - rrfidtemp_start;
                                char rrfidtemp[rrfidtemp_length + 1];
                                strncpy(rrfidtemp, rrfidtemp_start, rrfidtemp_length);
                                rrfidtemp[rrfidtemp_length] = '\0';

                                sprintf(temp.id1, "%02X%02X%02X%02X", temp.id1[8], temp.id1[9], temp.id1[10], temp.id1[11]);
                                sprintf(temp.id2, "%02X%02X%02X%02X", temp.id2[8], temp.id2[9], temp.id2[10], temp.id2[11]);
                                Usart1_Printf("id:%s,%s,%s,%s\n", (char *)temp.id1, lrfidtemp, (char *)temp.id2, rrfidtemp);

                                /*如果校验的数据和刚发送的相同，就表示发送成功，失败的话把蛋数据放到队列后面*/
                                if (strstr((char *)temp.id1, lrfidtemp) && strstr((char *)temp.id2, rrfidtemp))
                                {
                                    Usart1_Printf("test_good\n");
                                    temp.cnt = 0;
                                    //xQueueReceive( Lora_Buffer_Store_QueueHandle_t,&temp,0);
                                }
                                else//不同
                                {
                                    temp.cnt++;
                                    //Usart1_Printf("eggerror:%d\n", temp.cnt);
                                    if (temp.cnt <= 2)
                                    {
                                        xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                                    }
                                    else//错误次数大于2存到下一段时间再次发送
                                    {
                                        xQueueSend(Lora_Buffer_Store_QueueHandle_t, &temp, 0);
                                    }
                                    Lora_CLear_Lora_buffer(&temp);
                                }
                            }
                            else//未收到
                            {
                                temp.cnt++;
                                //Usart1_Printf("eggno:%d\n", temp.cnt);
                                if (temp.cnt <= 2)
                                {
                                    //temp.leave_time = 赋值现在时间
                                    xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                                }
                                else//错误次数大于2存到下一段时间再次发送，存入发送失败的储蛋队列
                                {
                                    temp.cnt = 0;
                                    xQueueSend(Lora_Buffer_Store_QueueHandle_t, &temp, 0);
                                }
                                Lora_CLear_Lora_buffer(&temp);
                            }
                            break;
                        }
												//接送错误校验
                        case (Error):
                        {
                            if (Lora_Struct.USART_Length != 0 && strstr((char *)Lora_Struct.USART_BUFF, "extype"))
                            {
                                //Usart1_Printf("\nextype ok\n");
                                /*提取后extype的数据存放在extypetemp*/
                                char *extypetemp_start = strstr(Lora_Struct.USART_BUFF, "\"extype\":") + 9;
                                char *extypetemp_end = strchr(extypetemp_start, '}');
                                size_t extypetemp_length = extypetemp_end - extypetemp_start;
                                char extypetemp[extypetemp_length + 1];
                                strncpy(extypetemp, extypetemp_start, extypetemp_length);
                                extypetemp[extypetemp_length] = '\0';
                                u8 u8_extypetemp = atoi(extypetemp);
                                //Usart1_Printf("test_u8_extypetemp:%d",u8_extypetemp);

                                /*如果校验的数据和刚发送的不同，发送次数加1，再次加入队列，超过三次不加*/
//                                if (u8_extypetemp != temp.error)
//                                {
//                                    temp.cnt++;
//                                    if (temp.cnt <= 2)
//                                    {
//                                        xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
//                                    }
//                                }
//                                else
//                                {
//                                    //Usart1_Printf("errok\n");
//                                }
                            }
                            else
                            {
                                temp.cnt++;
                                //Usart1_Printf("errno:%d\n", temp.cnt);
                                if (temp.cnt <= 2)
                                {
                                    xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                                }
                            }
                            break;
                        }
												
                        case (TakeTime):
                        {
                            if (Lora_Struct.USART_Length != 0 && strstr((char *)Lora_Struct.USART_BUFF, "taketime"))
                            {

                                char *timestamp_start = strstr(Lora_Struct.USART_BUFF, "\"taketime\":\"") + 12;
                                char *timestamp_end = strchr(timestamp_start, '\"');
                                size_t timestamp_length = timestamp_end - timestamp_start;
                                char timestamp[timestamp_length + 1];
                                strncpy(timestamp, timestamp_start, timestamp_length);
                                timestamp[timestamp_length] = '\0';
                                uint32_t u32_timestamp = last_delate_time;
                                last_delate_time = atoi(timestamp); //提取后最后捡蛋时间戳存放在u32_timestamp
																
																if (last_delate_time != u32_timestamp)//如果接收到的是最新的时间戳，通过和上次接收的对比不一样，就当作是最新的
                                {
                                    /*删除这个时间戳之前的蛋数据*/
                                    memset(queue, 0, sizeof(queue));
                                    queue_cnt = 0;
                                    //Usart1_Printf("delete\n");
                                    Lora_CLear_Lora_buffer(&temp);
                                }
                                else//没收到最新的
                                {
                                    temp.cnt++;
                                    //Usart1_Printf("ertime:%d\n", temp.cnt);
                                    if (temp.cnt <= 2)
                                    {
                                        xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                                    }
                                    Lora_CLear_Lora_buffer(&temp);
                                }
                            }
                            else//没收到
                            {
                                temp.cnt++;
                                //Usart1_Printf("notime:%d\n", temp.cnt);
                                if (temp.cnt <= 2)//3次都没有收到就下一次时间段发送
                                {
                                    xQueueSend(Lora_Buffer_Send_QueueHandle_t, &temp, 0);
                                }
                                Lora_CLear_Lora_buffer(&temp);
                            }
                            break;
                        }
                        default:
                        {
                            //printf("default\n");
                            break;
                        }
                        }
                    }
                }

                Lora_Clear_Struct(); //清楚缓冲区
            }
            Lora_Flag = 0;
            //Usart1_Printf("sendqueue%d,lastqueue%d\n", uxQueueMessagesWaiting(Lora_Buffer_Send_QueueHandle_t), uxQueueMessagesWaiting(Lora_Buffer_LastSend_QueueHandle_t));
        }
        else if ((LoraTask_Nowtime.tm_min - LoraTask_Lasttime) % 60 >= 2)//这个条件如果成立就是串口的缓冲区出现了错误的数据，需要清除掉
        {
            Lora_Clear_Struct();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

int AppTaskCreate(void)
{
    u8 xreturn = 0;

    xTaskCreate((TaskFunction_t)ReadCardTask,          /* 任务入口函数 */
                          (const char *)"ReadCardTask",          /* 任务名字 */
                          (uint16_t)1024,                         /* 任务栈大小 */
                          (void *)NULL,                          /* 任务入口函数参数 */
                          (UBaseType_t)5,                        /* 任务的优先级 */
                          (TaskHandle_t *)&ReadCardTask_Handle); /* 任务控制块指针 */
						  xreturn++;
						  
	xTaskCreate((TaskFunction_t)TotalTask,          /* 任务入口函数 */
                          (const char *)"TotalTask",          /* 任务名字 */
                          (uint16_t)1024,                         /* 任务栈大小 */
                          (void *)NULL,                          /* 任务入口函数参数 */
                          (UBaseType_t)5,                        /* 任务的优先级 */
                          (TaskHandle_t *)&TotalTask_Handle); /* 任务控制块指针 */	
						   xreturn++;						  

	xTaskCreate((TaskFunction_t)IDinandoutTask,          /* 任务入口函数 */
                          (const char *)"IDinandoutTask",          /* 任务名字 */
                          (uint16_t)1024,                         /* 任务栈大小 */
                          (void *)NULL,                          /* 任务入口函数参数 */
                          (UBaseType_t)3,                        /* 任务的优先级 */
                          (TaskHandle_t *)&IDinandoutTask_Handle); /* 任务控制块指针 */	
						   xreturn++;
						  
	xTaskCreate((TaskFunction_t)DatatestTask,          /* 任务入口函数 */
                          (const char *)"DatatestTask",          /* 任务名字 */
                          (uint16_t)512,                         /* 任务栈大小 */
                          (void *)NULL,                          /* 任务入口函数参数 */
                          (UBaseType_t)3,                        /* 任务的优先级 */
                          (TaskHandle_t *)&DatatestTask_Handle); /* 任务控制块指针 */	
						   xreturn++;
						  
	xTaskCreate((TaskFunction_t)PWM_START_Task,          /* 任务入口函数 */
                          (const char *)"PWM_START_Task",          /* 任务名字 */
                          (uint16_t)512,                         /* 任务栈大小 */
                          (void *)NULL,                          /* 任务入口函数参数 */
                          (UBaseType_t)4,                        /* 任务的优先级 */
                          (TaskHandle_t *)&PWM_START_Task_Handle); /* 任务控制块指针 */	
						   xreturn++;
						  
	xTaskCreate((TaskFunction_t)PWM_BACK_Task,          /* 任务入口函数 */
                          (const char *)"PWM_BACK_Task",          /* 任务名字 */
                          (uint16_t)512,                         /* 任务栈大小 */
                          (void *)NULL,                          /* 任务入口函数参数 */
                          (UBaseType_t)4,                        /* 任务的优先级 */
                          (TaskHandle_t *)&PWM_BACK_Task_Handle); /* 任务控制块指针 */	
						   xreturn++;	
													
													
	xTaskCreate((TaskFunction_t)ShowTask,          /* 任务入口函数 */
                          (const char *)"ShowTask",          /* 任务名字 */
                          (uint16_t)1024,                     /* 任务栈大小 */
                          (void *)NULL,                      /* 任务入口函数参数 */
                          (UBaseType_t)2,                    /* 任务的优先级 */
                          (TaskHandle_t *)&ShowTask_Handle); /* 任务控制块指针 */
								xreturn++;
						  
						  
	xTaskCreate((TaskFunction_t)LoraTask,          /* 任务入口函数 */
                          (const char *)"LoraTask",          /* 任务名字 */
                          (uint16_t)2048,                    /* 任务栈大小 */
                          (void *)NULL,                      /* 任务入口函数参数 */
                          (UBaseType_t)7,                    /* 任务的优先级 */
                          (TaskHandle_t *)&LoraTask_Handle); /* 任务控制块指针 */
								xreturn++;		
													
	return xreturn;
}


int main(void)
{
	Usart1_Init();
	Lora_Init();
	
	LVGL_mutex = xSemaphoreCreateMutex();
    Usart1_mutex = xSemaphoreCreateMutex();
    Flash_mutex = xSemaphoreCreateMutex();
    Eggbuf_mutex = xSemaphoreCreateMutex();
    Usart_lcd_mutex = xSemaphoreCreateMutex();
    Lora_mutex = xSemaphoreCreateMutex();
	
	
    Lora_Buffer_Store_QueueHandle_t = xQueueCreate(20, sizeof(Lora_Buffer));
    Lora_Buffer_Send_QueueHandle_t = xQueueCreate(20, sizeof(Lora_Buffer));
    Lora_Buffer_LastSend_QueueHandle_t = xQueueCreate(20, sizeof(Lora_Buffer));

		KEY_Init();
		TIM4_PWM_Init(2000-1,84-1);
		easyflash_init();
		
		
		Init_goose_queue(&goose_queue);
		Init_egg_queue(&egg_queue); // 初始化鹅蛋队列

		RFID_Init();  // 添加RFID初始化
		Usart_lcd_Init();
    Stop_Key_Init(); //停止按键初始化
		
 		//清除Flash中的历史数据
//    Egg_Queue_Reset();
//    Usart1_Printf("Egg queue historical data cleared\r\n");
		
		//初始化蛋队列，从Flash中加载数据
		Egg_Queue_Init();
		Usart1_Printf("Goose queue initialized from Flash\r\n");
		// 显示从Flash中加载的蛋队列信息
		Usart1_Printf("=== Egg QUEUE FROM FLASH ===\r\n");
		for (int i = 0; i < egg_queue_array.Egg_queue_length; i++) 
		{
				Usart1_Printf("Goose %d: ID=", i + 1);
				for (int j = 0; j < 12; j++) 
				{ 
						Usart1_Printf("%02X", egg_queue_array.Egg_Queue_data[i].id[j]);
				}
				//Usart1_Printf(", intime=%lu, goose_count=%lu\r\n", egg_queue_array.Egg_Queue_data[i].egg_timestamp,egg_queue_array.Egg_Queue_data[i].goose_count);
						Usart1_Printf("\r\n");
						Usart1_Printf("egg_intime : %lu\r\n",  egg_queue_array.Egg_Queue_data[i].egg_intime);      //入窝时间
						Usart1_Printf("egg_outtime: %lu\r\n",  egg_queue_array.Egg_Queue_data[i].egg_outtime);     //出窝时间
						Usart1_Printf("goose_count: %lu\r\n", egg_queue_array.Egg_Queue_data[i].goose_count);     //产蛋时鹅数量
						Usart1_Printf("egg_timestamp: %lu\r\n",  egg_queue_array.Egg_Queue_data[i].egg_timestamp);   //产蛋时间戳
						Usart1_Printf("\r\n"); 
						
				
				// 添加到内存队列
				if (Egg_Queue_Flag && ADD_egg_queue(&egg_queue)) 
				{
						egger new_egg = egg_queue.tail;
						memcpy(new_egg->id, egg_queue_array.Egg_Queue_data[i].id, ID_LENGTH);
						new_egg->egg_intime = egg_queue_array.Egg_Queue_data[i].egg_intime;
						new_egg->egg_outtime = egg_queue_array.Egg_Queue_data[i].egg_outtime;
						new_egg->goose_count = egg_queue_array.Egg_Queue_data[i].goose_count;
						new_egg->egg_timestamp = egg_queue_array.Egg_Queue_data[i].egg_timestamp;
				} 
				
				else 
				{
						Usart1_Printf("Failed to add egg from Flash to memory queue!\r\n");
				}
				
				
		}
				
		
		

    if(AppTaskCreate() == TASKNUM) 
		{
        Usart1_Printf("Tasks created successfully\r\n");
    } 

		
     //串口初始化115200 8-N-1，中断接收
                   // Delay_Init();

                   //    按键测试
                   //    Stop_Key_Init();        //停止按键初始化
                   //    while (1)
                   //    {
                   //        if(Step1_Key.start_key_state)
                   //        {
                   //            printf("KEY1\n");
                   //        }
                   //        if(Step1_Key.end_key_state)
                   //        {
                   //            printf("KEY2\n");
                   //        }
                   //        if(TakeEgg_Key.start_key_state)
                   //        {
                   //            printf("KEY3\n");
                   //        }
    /* 启动任务调度 */
    vTaskStartScheduler(); /* 启动任务，开启调度 */
    while (1)
	{
		
	}; /* 正常不会执行到这里 */
}




