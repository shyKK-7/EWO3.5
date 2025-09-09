#include "stm32f4xx.h"
#include "./RTC/bsp_rtc.h"
#include "./usart/bsp_usart.h"

 
/**
  * @brief  设置时间和日期
  * @param  无
  * @retval 无
  */
void RTC_TimeAndDate_Set(void)
{
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	u32 temp;
    temp = RTC_ReadBackupRegister(RTC_BKP_DRX);
    if (temp != RTC_BKP_DATA)
    {
        printf("RTC RESET TIME\n");
        // 初始化时间
        RTC_TimeStructure.RTC_H12 = RTC_H12_AMorPM;
        RTC_TimeStructure.RTC_Hours = HOURS;        
        RTC_TimeStructure.RTC_Minutes = MINUTES;      
        RTC_TimeStructure.RTC_Seconds = SECONDS;      
        RTC_SetTime(RTC_Format_BINorBCD, &RTC_TimeStructure);
        RTC_WriteBackupRegister(RTC_BKP_DRX, RTC_BKP_DATA);
        // 初始化日期	
        RTC_DateStructure.RTC_WeekDay = WEEKDAY;       
        RTC_DateStructure.RTC_Date = DATE;         
        RTC_DateStructure.RTC_Month = MONTH;         
        RTC_DateStructure.RTC_Year = YEAR;        
        RTC_SetDate(RTC_Format_BINorBCD, &RTC_DateStructure);
        RTC_WriteBackupRegister(RTC_BKP_DRX, RTC_BKP_DATA);
    }
}
void MyRTC_SetTime(u8 year, u8 month, u8 date, u8 day, u8 hour, u8 min, u8 sec)
{
    RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
 // 初始化时间
    RTC_TimeStructure.RTC_H12 = RTC_H12_AMorPM;
    RTC_TimeStructure.RTC_Hours = hour + 8;      
    RTC_TimeStructure.RTC_Minutes = min;      
    RTC_TimeStructure.RTC_Seconds = sec;      
    RTC_SetTime(RTC_Format_BINorBCD, &RTC_TimeStructure);
    // 初始化日期	
    RTC_DateStructure.RTC_WeekDay = day;       
    RTC_DateStructure.RTC_Date = date;         
    RTC_DateStructure.RTC_Month = month;         
    RTC_DateStructure.RTC_Year = year;        
    RTC_SetDate(RTC_Format_BINorBCD, &RTC_DateStructure);
}

/**
  * @brief  RTC配置：选择RTC时钟源，设置RTC_CLK的分频系数
  * @param  无
  * @retval 无
  */
void RTC_CLK_Config(void)
{  
	RTC_InitTypeDef RTC_InitStructure;
	
	/*使能 PWR 时钟*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  /* PWR_CR:DBF置1，使能RTC、RTC备份寄存器和备份SRAM的访问 */
  PWR_BackupAccessCmd(ENABLE);

#if defined (RTC_CLOCK_SOURCE_LSI) 
  /* 使用LSI作为RTC时钟源会有误差 
	 * 默认选择LSE作为RTC的时钟源
	 */
  /* 使能LSI */ 
  RCC_LSICmd(ENABLE);
  /* 等待LSI稳定 */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {
  }
  /* 选择LSI做为RTC的时钟源 */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);

#elif defined (RTC_CLOCK_SOURCE_LSE)

  /* 使能LSE */ 
  RCC_LSEConfig(RCC_LSE_ON);
   /* 等待LSE稳定 */   
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }
  /* 选择LSE做为RTC的时钟源 */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);    

#endif /* RTC_CLOCK_SOURCE_LSI */

  /* 使能RTC时钟 */
  RCC_RTCCLKCmd(ENABLE);

  /* 等待 RTC APB 寄存器同步 */
  RTC_WaitForSynchro();
   
/*=====================初始化同步/异步预分频器的值======================*/
	/* 驱动日历的时钟ck_spare = LSE/[(255+1)*(127+1)] = 1HZ */
	
	/* 设置异步预分频器的值 */
	RTC_InitStructure.RTC_AsynchPrediv = ASYNCHPREDIV;
	/* 设置同步预分频器的值 */
	RTC_InitStructure.RTC_SynchPrediv = SYNCHPREDIV;	
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24; 
	/* 用RTC_InitStructure的内容初始化RTC寄存器 */
	if (RTC_Init(&RTC_InitStructure) == ERROR)
	{
		Usart1_Printf("---------------------------------RTC INIT ERROR\r\n");
	}	
    else
    {
        Usart1_Printf("RTC INIT SUCCESS\n");
    }
}

/**
  * @brief  RTC配置：选择RTC时钟源，设置RTC_CLK的分频系数
  * @param  无
  * @retval 无
  */
#define LSE_STARTUP_TIMEOUT     ((uint16_t)0x05000)
void RTC_CLK_Config_Backup(void)
{  
  __IO uint16_t StartUpCounter = 0;
	FlagStatus LSEStatus = RESET;	
	RTC_InitTypeDef RTC_InitStructure;
	
	/* 使能 PWR 时钟 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  /* PWR_CR:DBF置1，使能RTC、RTC备份寄存器和备份SRAM的访问 */
  PWR_BackupAccessCmd(ENABLE);

/*=========================选择RTC时钟源==============================*/
/* 默认使用LSE，如果LSE出故障则使用LSI */
  /* 使能LSE */
  RCC_LSEConfig(RCC_LSE_ON);	
	
	/* 等待LSE启动稳定，如果超时则退出 */
  do
  {
    LSEStatus = RCC_GetFlagStatus(RCC_FLAG_LSERDY);
    StartUpCounter++;
  }while((LSEStatus == RESET) && (StartUpCounter != LSE_STARTUP_TIMEOUT));
	
	
	if(LSEStatus == SET )
  {
		Usart1_Printf("\n\r LSE 启动成功 \r\n");
		/* 选择LSE作为RTC的时钟源 */
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  }
	else
	{
		Usart1_Printf("\n\r LSE 故障，转为使用LSI \r\n");
		
		/* 使能LSI */	
		RCC_LSICmd(ENABLE);
		/* 等待LSI稳定 */ 
		while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
		{			
		}
		
		Usart1_Printf("\n\r LSI 启动成功 \r\n");
		/* 选择LSI作为RTC的时钟源 */
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	}
	
  /* 使能 RTC 时钟 */
  RCC_RTCCLKCmd(ENABLE);
  /* 等待 RTC APB 寄存器同步 */
  RTC_WaitForSynchro();

/*=====================初始化同步/异步预分频器的值======================*/
	/* 驱动日历的时钟ck_spare = LSE/[(255+1)*(127+1)] = 1HZ */
	
	/* 设置异步预分频器的值为127 */
	RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
	/* 设置同步预分频器的值为255 */
	RTC_InitStructure.RTC_SynchPrediv = 0xFF;	
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24; 
	/* 用RTC_InitStructure的内容初始化RTC寄存器 */
	if (RTC_Init(&RTC_InitStructure) == ERROR)
	{
		Usart1_Printf("\n\r RTC 时钟初始化失败 \r\n");
	}	

}

void MyRTC_Init(void)
{
//    #if USE_FLASH_INIT
//    u8 temp;
//    SPI_FLASH_BufferRead(&temp, CHECKADDR, 1);      //读取w25q16，查看是否已经被初始化过
//    if (temp == CHECKNUM)
//    {
//        Usart1_Printf("RTC already set time\n");
//    }
//    else
//    {
//        Usart1_Printf("RTC set time\n");
//        temp = CHECKNUM;
//        SPI_FLASH_BufferWrite(&temp, CHECKADDR, 1);  //写w25q16，表明已经被初始化过
//        RTC_TimeAndDate_Set();
//    }
//    #else
//    Usart1_Printf("RTC set time\n");
//    RTC_TimeAndDate_Set();
//    #endif
    RTC_CLK_Config();
    //RTC_CLK_Config_Backup();
    RTC_TimeAndDate_Set();
    
}
u32 MyRTC_GetTimestamp(void)
{
    struct tm tm_new;

	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	// 获取日历
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	
	tm_new.tm_sec = RTC_TimeStructure.RTC_Seconds;
	tm_new.tm_min = RTC_TimeStructure.RTC_Minutes;
	tm_new.tm_hour = RTC_TimeStructure.RTC_Hours;   
	tm_new.tm_mday = RTC_DateStructure.RTC_Date;
	tm_new.tm_mon = RTC_DateStructure.RTC_Month - 1;
	tm_new.tm_year = RTC_DateStructure.RTC_Year + 100;
	
	(void)RTC->DR;
	
	return mktime(&tm_new);
}
//还原成unix时间戳
u32 MyRTC_GetUnixTimestamp(void)
{
    struct tm tm_new;

	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	// 获取日历
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	
	tm_new.tm_sec = RTC_TimeStructure.RTC_Seconds;
	tm_new.tm_min = RTC_TimeStructure.RTC_Minutes;
	tm_new.tm_hour = RTC_TimeStructure.RTC_Hours;   //还原成unix时间戳
	tm_new.tm_mday = RTC_DateStructure.RTC_Date;
	tm_new.tm_mon = RTC_DateStructure.RTC_Month - 1;
	tm_new.tm_year = RTC_DateStructure.RTC_Year + 100;
	
	(void)RTC->DR;
	
	return mktime(&tm_new);
}
struct tm MyRTC_GetTime(void)
{
    struct tm tm_new;;
    
    RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	// 获取日历
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	
	tm_new.tm_sec = RTC_TimeStructure.RTC_Seconds;
	tm_new.tm_min = RTC_TimeStructure.RTC_Minutes;
	tm_new.tm_hour = RTC_TimeStructure.RTC_Hours - 8;
    if (tm_new.tm_hour < 0)
    {
        tm_new.tm_hour += 24;
    }
    if (tm_new.tm_hour >= 24)
    {
        tm_new.tm_hour -= 24;
    }
	tm_new.tm_mday = RTC_DateStructure.RTC_Date;
	tm_new.tm_mon = RTC_DateStructure.RTC_Month - 1;
	tm_new.tm_year = RTC_DateStructure.RTC_Year + 100;
    return tm_new;
}

void Timestamp_Set_RTC_Time(long long timestamp)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef DateToUpdate = {0};

    // 处理时间戳（单位：秒），加上东8区时间偏移（北京时间）
    timestamp += 8 * 60 * 60; // 东8区，北京时间

    struct tm timeinfo;
    localtime_r((const time_t *)&timestamp, &timeinfo);

    // 提取年月日时分秒
    int year = timeinfo.tm_year + 1900;   // tm_year 是从1900年开始，需加1900
    int month = timeinfo.tm_mon + 1;       // tm_mon 是从0开始，需加1
    int day = timeinfo.tm_mday;
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;
    int second = timeinfo.tm_sec;
    int week = 0;

    // 计算星期几（基姆拉尔森公式）
    if (month == 1 || month == 2)
    {
        month += 12;
        year--;
    }

    // 基姆拉尔森公式
    week = (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;

    week = (week + 1) % 7;  // 星期天为0，调整为星期一为1，星期六为7

    // 输出调试信息（可选）
    // printf("timestamp: %lld\n", timestamp);
    // printf("year:%d month:%d day:%d hour:%d minute:%d second:%d\n", year, month, day, hour, minute, second);    

    // 处理年份（RTC中使用的是2位数的年份）
    year %= 100;

    // 设置RTC时间
    sTime.RTC_Hours = hour;
    sTime.RTC_Minutes = minute;
    sTime.RTC_H12 = RTC_H12_AMorPM;  // 假设AM，实际可以根据需要修改
    sTime.RTC_Seconds = second;
    RTC_SetTime(RTC_Format_BINorBCD, &sTime);

    // 设置RTC日期
    DateToUpdate.RTC_WeekDay = week;
    DateToUpdate.RTC_Month = month;
    DateToUpdate.RTC_Date = day;
    DateToUpdate.RTC_Year = year;

    RTC_SetDate(RTC_Format_BINorBCD, &DateToUpdate);
}
struct tm MyRTC_Lora_GetTime(void)
{
    struct tm tm_new;;
    
    RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	// 获取日历
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	
	tm_new.tm_sec = RTC_TimeStructure.RTC_Seconds;
	tm_new.tm_min = RTC_TimeStructure.RTC_Minutes;
	tm_new.tm_hour = RTC_TimeStructure.RTC_Hours;
    if (tm_new.tm_hour < 0)
    {
        tm_new.tm_hour += 24;
    }
    if (tm_new.tm_hour >= 24)
    {
        tm_new.tm_hour -= 24;
    }
	tm_new.tm_mday = RTC_DateStructure.RTC_Date;
	tm_new.tm_mon = RTC_DateStructure.RTC_Month - 1;
	tm_new.tm_year = RTC_DateStructure.RTC_Year + 100;
    return tm_new;
}
/**********************************END OF FILE*************************************/
