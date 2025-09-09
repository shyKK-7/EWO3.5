#ifndef __RTC_H__
#define __RTC_H__

#include "stm32f4xx.h"
#include "time.h"


// 时钟源宏定义
#define RTC_CLOCK_SOURCE_LSE      
//#define RTC_CLOCK_SOURCE_LSI

// 异步分频因子
#define ASYNCHPREDIV         0X7F
// 同步分频因子
#define SYNCHPREDIV          0XFF


#define USE_FLASH_INIT       1

// 时间宏定义
#define RTC_H12_AMorPM			 RTC_H12_AM  
#define HOURS                21          // 0~23 8小时时差
#define MINUTES              52         // 0~59
#define SECONDS              00          // 0~59

// 日期宏定义
#define WEEKDAY              2         // 1~7
#define DATE                 13         // 1~31
#define MONTH                8         // 1~12
#define YEAR                 24         // 0~99

// 时间格式宏定义
#define RTC_Format_BINorBCD  RTC_Format_BIN

// 备份域寄存器宏定义
#define RTC_BKP_DRX          RTC_BKP_DR0
// 写入到备份寄存器的数据宏定义
#define RTC_BKP_DATA         0x2227
                       
struct time 
{
    u16 y;
    u8 m;
    u8 d;
    u8 HH;
    u8 MM;
    u8 SS;
};
                                 
void RTC_CLK_Config(void);
void RTC_TimeAndDate_Set(void);
void RTC_TimeAndDate_Show(void);
u32 MyRTC_GetTimestamp(void);
u32 MyRTC_GetUnixTimestamp(void);
void MyRTC_Init(void);
struct tm MyRTC_GetTime(void);
void MyRTC_SetTime(u8 year, u8 month, u8 date, u8 day, u8 hour, u8 min, u8 sec);
void Timestamp_Set_RTC_Time(long long timestamp);
struct tm MyRTC_Lora_GetTime(void);

#endif // __RTC_H__
