#ifndef __MAIN_H
#define __MAIN_H

/*包含的头文件*/

#include "stm32f4xx.h"                  // Device header
#include <time.h>
#include <stdlib.h>
#include "String.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "stdlib.h"

#include "./RTC/bsp_rtc.h"
#include "./usart/bsp_usart.h"
#include "./delay/delay.h"
#include "./hx711/hx711.h"
#include "./step_motor/step_motor.h"
#include "./stop_key/stop_key.h"
#include "./rfid_r200/rfid_r200.h"
#include "./as5600/as5600.h"
#include "./spi_flash/spi_flash.h"
#include "./usart_lcd/usart_lcd.h"
#include "./struct/struct.h"
#include "./iwdg/iwdg.h"
#include "./lora/lora.h"
#include "easyflash.h"
#include "./base64/base64.h"
#include "./KEY/key.h"
#include "./duoji/duoji.h"
#include "./sys/sys.h"
//struct record                     //记录信息结构体，用于上电恢复
//{
//    u8 ID[ID_LENGTH],
//    u8 totalstate,
//    u8 
//    
//}rec;



/*宏定义*/

#define TASKNUM 6                 //FreeRTOS任务数，用这个来判断任务是否创建成功
#define MAXLENGTH 8                 //队列最大长度
   
#endif
