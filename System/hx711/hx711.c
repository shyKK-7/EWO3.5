#include "./hx711/hx711.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "String.h"

extern struct device dev;
u32 HX711_Parameter;    //称重系数
u32 HX711_Buffer;       //原始值
s32 Weight_before;      

/*增加重量校准功能*/

u8 Hx711_GetParameter(void)
{
    HX711_Parameter = ef_get_int("HX711_Parameter");
    if (HX711_Parameter)
    {
        return 1;
    }
    return 0;
}


/*校准专用*/
u32 HX711_Read_adj(void)
{
	u32 count; 
	u8 i; 
    u32 timeout = 0;
//    if (xSemaphoreTake(HX711_mutex, portMAX_DELAY) == pdTRUE)
//    {
        GPIO_WriteBit(HX711_PORT,HX711_DOUT_PIN,Bit_SET);//  	HX711_DOUT=1; 
        Delay_us(1);
        GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_RESET);//			HX711_SCK=0; 
        count=0;	
        while(GPIO_ReadInputDataBit(HX711_PORT,HX711_DOUT_PIN)==1)
        {
        //    vTaskDelay(pdMS_TO_TICKS(1));
            Delay_ms(1);
            timeout++;
            if (timeout > 500)
            {
                timeout = 0;
                printf("--------------------HX711 TIMEOUT ERROR\n");
                return 0;
            }
        }
        for(i=0;i<24;i++)
        { 
            GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_SET);		//HX711_SCK=1;		
            count=count<<1; 
            Delay_us(1);
            GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_RESET);		//HX711_SCK=0; 
            if(GPIO_ReadInputDataBit(HX711_PORT,HX711_DOUT_PIN))
            {
                count++;
            }			 
            Delay_us(1);
        } 
        GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_SET);			//HX711_SCK=1; 
        count=count^0x800000;
        Delay_us(1);
        GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_RESET);			//HX711_SCK=0;  
//        xSemaphoreGive(HX711_mutex);
//    }
    return(count);
}


u32 Hx711_Read_MedianValue_adj(void)
{
    u32 values[SAMPLES];

    // 采集数据
    for (int i = 0; i < SAMPLES; i++)
    {
        values[i] = HX711_Read_adj();
        Delay_ms(100);
    }
    // 冒泡排序
    for (int i = 0; i < SAMPLES - 1; i++)
    {
        for (int j = 0; j < SAMPLES - 1 - i; j++)
        {
            if (values[j] > values[j + 1])
            {
                long temp = values[j];
                values[j] = values[j + 1];
                values[j + 1] = temp;
            }
        }
    }

    // 取中值
    return values[SAMPLES / 2 - 2];
}

u32 Hx711_Read_MedianValue(void)
{
    u32 values[SAMPLES];

    // 采集数据
    for (int i = 0; i < SAMPLES; i++)
    {
        values[i] = HX711_Read();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // 冒泡排序
    for (int i = 0; i < SAMPLES - 1; i++)
    {
        for (int j = 0; j < SAMPLES - 1 - i; j++)
        {
            if (values[j] > values[j + 1])
            {
                long temp = values[j];
                values[j] = values[j + 1];
                values[j + 1] = temp;
            }
        }
    }

    // 取中值
    return values[SAMPLES / 2];
}

void Hx711_AdjustParameter(void)
{
    u32 weight_now = 0, weight_before = 0;
    int weight_change = 0;
    printf("Start Adjust HX711\n");
    Usart_lcd_adjust_adjust();
    weight_now = Hx711_Read_MedianValue_adj();
    while (1)
    {
        weight_before = weight_now;
        weight_now = Hx711_Read_MedianValue_adj();
//        weight_now = HX711_Read_adj();
//        printf("weight_now:%d\n", weight_now);
//        printf("weight_before:%d\n", weight_before);
        weight_change = weight_before - weight_now;
        printf("weight_change:%d\n", weight_change);
        if (weight_change > 10000)
        {
            Usart_lcd_adjust_adjusting();
            printf("Weight Change\n");
            Delay_ms(4000);//延时4s
            weight_now = Hx711_Read_MedianValue_adj();
            weight_change = 1.0 * weight_before - weight_now;
            HX711_Parameter = weight_change / 255;
            printf("HX711_Parameter:%d\n", HX711_Parameter);
            break;
        }
//        Delay_ms(1000);//延时1s
    }
    Usart_lcd_adjust_success();
    /*等待将100g拿出*/
    weight_before = Hx711_Read_MedianValue_adj();
    while (1)
    {
        weight_now = Hx711_Read_MedianValue_adj();
        weight_change = weight_now - weight_before;
        printf("weight_change:%d\n", weight_change);
        if (weight_change > 10000 || weight_change < -10000)
        {
            break;
        }
    }
    ef_set_int("HX711_Parameter", HX711_Parameter);
    printf("SYSTEM RESET\n");
    NVIC_SystemReset();// 复位
}

void Hx711_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(Hx711_GPIO_CLK, ENABLE); //GPIO 时钟使能
    GPIO_InitStructure.GPIO_Pin = HX711_DOUT_PIN;    
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed; //速度 50MHz
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;     //上拉输入
    GPIO_Init(HX711_PORT, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = HX711_SCK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
    GPIO_Init(HX711_PORT, &GPIO_InitStructure);
    
	
	HX711_SCK(0);				
}

u32 HX711_Read(void)	//
{
	u32 count; 
	u8 i; 
    u32 timeout = 0;
//    if (xSemaphoreTake(HX711_mutex, portMAX_DELAY) == pdTRUE)
//    {
        GPIO_WriteBit(HX711_PORT,HX711_DOUT_PIN,Bit_SET);//  	HX711_DOUT=1; 
        Delay_us(1);
        GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_RESET);//			HX711_SCK=0; 
        count=0;	
        while(GPIO_ReadInputDataBit(HX711_PORT,HX711_DOUT_PIN)==1)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
            timeout++;
            if (timeout > 500)
            {
                timeout = 0;
                printf("--------------------HX711 TIMEOUT ERROR\n");
                dev.error = ERROR_HX711;
                return 0;
            }
        }
        for(i=0;i<24;i++)
        { 
            GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_SET);		//HX711_SCK=1;		
            count=count<<1; 
            Delay_us(1);
            GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_RESET);		//HX711_SCK=0; 
            if(GPIO_ReadInputDataBit(HX711_PORT,HX711_DOUT_PIN))
            {
                count++;
            }			 
            Delay_us(1);
        } 
        GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_SET);			//HX711_SCK=1; 
        count=count^0x800000;
        Delay_us(1);
        GPIO_WriteBit(HX711_PORT,HX711_SCK_PIN,Bit_RESET);			//HX711_SCK=0;  
//        xSemaphoreGive(HX711_mutex);
//    }
    return(count);
}


s32 Get_Weight(void)
{
	s32 Weight_Shiwu = 0;
    HX711_Buffer = Hx711_Read_MedianValue();
    Weight_Shiwu = HX711_Buffer/HX711_Parameter;//5.415; 

	return Weight_Shiwu;
}

s32 Get_StableWeight(void)//获取稳定重量，若1s内重量没有发生明显变化，则获得到的重量为稳定重量
{
    s32 StableWeight[STABLE_TIMES];
    s32 Weight_Shiwu;
    s32 sum;
    u8 i, flag;
    while(1)
    {
        sum = 0;
        flag = 0;
        for (i = 0; i < STABLE_TIMES; i++)
        {
            StableWeight[i] = Get_Weight();
            vTaskDelay(pdMS_TO_TICKS(INTERVAL_TIME));
        }
        for (i = 0; i < STABLE_TIMES; i++)
        {
            sum += StableWeight[i];
        }
        for (i = 0; i < STABLE_TIMES; i++)
        {
            if (sum / STABLE_TIMES - StableWeight[i] > 1 || sum / STABLE_TIMES - StableWeight[i] < -1)
            {
                flag = 1;
            }
        }
        if (flag == 0)
        {
            break;
        }
        else
        {
            memset(StableWeight, 0, sizeof(s32) * STABLE_TIMES);
        }
    }
    Weight_Shiwu = sum / STABLE_TIMES;				
    return Weight_Shiwu;
}

void Hx711_Init(void)
{
	Hx711_Config();
    if (!Hx711_GetParameter())
    {
        Hx711_AdjustParameter();

    }
    printf("HX711 Init Success\n");
}
