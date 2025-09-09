#ifndef __BASE64_H__
#define __BASE64_H__

#include <stdio.h>
#include <stdint.h>
#include "./struct/struct.h"
#include "main.h"
#include "easyflash.h"

// 强制结构体按 1 字节对齐
#pragma pack(1)

struct data {
    unsigned int rfid_1;        //脚环1 8位
    unsigned int rfid_2;        //脚环2 8位
    unsigned short egg_weight;  //蛋重
    unsigned int egg_intime;    //入窝时间
    unsigned int egg_outtime;  //产蛋时间
};

#pragma pack()  // 恢复默认对齐方式

//void base64_convertegg(struct egg *p, u8 length);                                                 //转换蛋信息结构体到data结构体
void base64_encode(char *output);                             //获取data数组的Base64编码结果函数
void base64_convertegg(struct egg_info *p, u8 length);

#endif
