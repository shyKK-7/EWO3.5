#ifndef __STRUCT_H
#define __STRUCT_H


#include "main.h"
#include "stm32f4xx.h"                  // Device header
#include "./rfid_r200/rfid_r200.h"
/*这里主要定义一些主函数用到的结构体和枚举类型*/

#define LENGTH_ID  12

typedef enum DEVSTATE                          //装置运行状态宏定义
{
    READCARD = 0, 
    WAIT, 
    PUSHEGG,
    WEIGHEGG,
    ERR,
}DEVSTATE;

typedef struct goose
{
	u8 id [LENGTH_ID];				//脚环ID
    u32 egg_intime;                 //入窝时间
    u32 egg_outtime;				//产蛋时间
	struct goose *next;						//读指针
}goose,*gooser;

typedef struct
{
	gooser head;
	gooser tail;
}Goose_queue;

// 鹅蛋数据结构
typedef struct egg_info
{
    u8 id[LENGTH_ID];           // 序号为1的鹅ID（队头鹅的ID）
    u32 egg_intime;             // 入窝时间
    u32 egg_outtime;            // 出窝时间
    u32 goose_count;             // 产蛋时鹅的数量
    u32 egg_timestamp;          // 产蛋时间戳
    struct egg_info *next;      // 读指针
} egg_info, *egger;

// 鹅蛋队列结构
typedef struct
{
    egger head;
    egger tail;
} Egg_queue;


struct egg                          //蛋信息结构体
{
    u8 id1[LENGTH_ID];              //脚环ID1
    u8 id2[LENGTH_ID];              //脚环ID2    
    u16 weight;                     //蛋重
    u32 egg_intime;                 //入窝时间
    u32 egg_outtime;                //产蛋时间
};                         

typedef enum DEVERROR                          //错误宏定义
{
    NOERROR = 0,
    ERROR_MOTOR,                    //电机错误
    ERROR_HX711,                    //HX711无应答错误
    ERROR_LCD,                      //LCD无应答错误
    ERROR_RFID,                     //RFID无应答错误
    ERROR_IWDG,                     //RFID无应答错误
    ERROR_WEIGHT,                   //重量异常错误
    ERROR_OTHERIN,                  //杂物入槽错误
    ERROR_DESERTID,                 //脚环掉落错误    
    ERROR_NEST,                     //趴窝错误
    ERROR_FULL,                     //蛋槽满蛋错误
    ERROR_NET,                      //网络通讯错误    
}DEVERROR;


struct device                       //装置运行状态结构体
{
	
//    DEVSTATE totalstate;               //总产蛋任务运行状态，用于指示当前装置的运行状态与保存   
//    DEVERROR error;                 //装置错误标志位
//    u8 flag_eggadd;                 //蛋槽入蛋标志位，有2个状态，当被置1时，表示称重完成，有蛋进入
//    u8 flag_noegg;                  //没有产蛋标志位，当这个标志位被置1对应母鹅没有产蛋
//    u8 flag_IDERROR;                //等待超时标志位，表示装置能一直检测到脚环, 当这个标志位被置1时，可能表示有脚环脱落在鹅窝内或者其他情况
//    u8 flag_readid1;                //id1读取标志位，当这个标志位被置1，表示id1已经读取到，现在读取id2
//    u8 flag_singgeID;               //单脚环标志位，当鹅脚环脱落一个的时候置1，仍然需要保存这个鹅的产蛋数据
//    u8 flag_saveegg;                //保存蛋数据标志位，当要保存有蛋数据时，就置这个标志位为1,RecordDataTask任务会去处理
//    u8 flag_errortake;              //误取蛋标志位，置1时要求工人将蛋放回
//    u8 flag_goosein;                //有鹅在鹅窝标志位，置1时表明有鹅在鹅窝，需要停止电机运行
//    u8 flag_motormove;              //电机运行标志位，当需要电机运行，就把这个标志位置1
//    u8 flag_gooseout;               //鹅离开鹅窝标志位，置1表示需要获取鹅离开后的重量
//    u8 flag_takeegg;                //取蛋按钮按下标志位，置1表示取蛋按钮被按下	

    DEVSTATE totalstate;               //总产蛋任务运行状态，用于指示当前装置的运行状态与保存   
    DEVERROR error;                 //装置错误标志位
    u8 flag_noegg;                  //没有产蛋标志位，当这个标志位被置1对应母鹅没有产蛋
    u8 flag_readid1;                //id1读取标志位，当这个标志位被置1，表示id1已经读取到，现在读取id2
    u8 flag_singgeID;               //单脚环标志位，当鹅脚环脱落一个的时候置1，仍然需要保存这个鹅的产蛋数据
    u8 flag_saveegg;                //保存蛋数据标志位，当要保存有蛋数据时，就置这个标志位为1,RecordDataTask任务会去处理
    u8 flag_errortake;              //误取蛋标志位，置1时要求工人将蛋放回             
    u8 flag_motormove;              //电机运行标志位，当需要电机运行，就把这个标志位置1
    u8 flag_gooseout;               //鹅离开鹅窝标志位，置1表示需要获取鹅离开后的重量
    u8 flag_takeegg;                //取蛋按钮按下标志位，置1表示取蛋按钮被按下
	
	u8 flag_totalstate;				  //总产蛋任务运行状态，用于指示当前装置的运行状态与保存
	u8 flag_IDERROR;				  //等待超时标志位，表示装置能一直检测到脚环, 当这个标志位被置1时，可能表示有脚环脱落在鹅窝内或者其他情况
	u8 flag_eggadd;					  //蛋槽入蛋标志位，置1时需要记录序号为1的鹅离开鹅窝的时间戳
	u8 flag_doortouch;				  //闸门被一定时间接触时，则判定为鹅蛋准备入槽，置1则让舵机转动
	u8 flag_NO1_leave;   			  //序号为1的鹅离开之后，收集完离开的时间戳，全部数据记录完毕，保存数据于对应的蛋中
	u8 flag_goosein;				  //rfid检测到鹅窝内有鹅
	u8 flag_gooses; 				  //多只鹅存在于鹅窝内
	u8 flag_haveread;				  //已经检测到第一只鹅
	u8 flag_egg_ready;				  //蛋准备进入标志位，按键按下10秒后置1
};



#endif
