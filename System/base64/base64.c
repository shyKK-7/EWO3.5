#include "./base64/base64.h"

// 强制结构体按 1 字节对齐
#pragma pack(1)

struct data eggs[MAXLENGTH];
u8 data_length;
#pragma pack()  // 恢复默认对齐方式


const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void base64_convertegg(egg_info *p, u8 length)
{
    u8 i, j;
    u8 id[4];   //原始后8位ID数组
    u32 id_number;          //转换为数字的id
    data_length = length;
    for (i = 0; i < length; i++)
    {
        //将id1和id2转换为u32数字
        //拷贝id1
        memcpy(id, p[i].id + 8, 4);
        for (j = 0; j < 4; j++) 
        {
            id_number |= (id[j] << (8 * (3 - j)));
        }
        eggs[i].rfid_1 = id_number;
        id_number = 0;
        memset(id, 0, 4);   //清空id数组
        
        //拷贝id2 - 使用实际的蛋信息而不是硬编码
        memcpy(id, p[i].id + 8, 4);
        for (j = 0; j < 4; j++) 
        {
            id_number |= (id[j] << (8 * (3 - j)));
        }
        eggs[i].rfid_2 = id_number;
        id_number = 0;
        
        //拷贝数量、入窝时间
        eggs[i].egg_weight = p[i].goose_count;
        eggs[i].egg_intime = p[i].egg_intime;
        eggs[i].egg_outtime = p[i].egg_outtime;
    }
}

// Base64 编码函数
void base64_encode(char *output) 
{
    u8 data[256];
    u16 cnt = 0;
    u32 lora_id;
    int i = 0, j = 0, n;
    uint8_t byte3[3] = {0};
    uint8_t byte4[4] = {0};
	
//	Lora_NID[0]='0';
//	Lora_NID[1]='0';
//	Lora_NID[2]='1';
//	Lora_NID[3]='8';
//	Lora_NID[4]='D';
//	Lora_NID[5]='0';
//	Lora_NID[6]='5';
//	Lora_NID[7]='7';
//	Lora_NID[8] = '\0';
    lora_id = ef_get_int("dev_lora_id");
    lora_id = strtoul(Lora_NID, NULL, 16);
    printf("qr lora_id:%X\n", lora_id);
    memcpy(data, &lora_id, sizeof(lora_id));
    for (n = 0; n < data_length; n++)
    {
        printf("id1:0X%X\nid2:0X%X\nweight:%d\nintime:%d\nouttime:%d\n", eggs[n].rfid_1, eggs[n].rfid_2, eggs[n].egg_weight, eggs[n].egg_intime, eggs[n].egg_outtime);
    }
    memcpy(data + sizeof(lora_id), eggs, sizeof(struct data) * data_length);
    size_t input_length = data_length * sizeof(struct data) + sizeof(lora_id);
    // 处理每3个字节，生成4个Base64字符
    while (input_length--) {
        byte3[i++] = data[cnt++];
        if (i == 3) {
            byte4[0] = (byte3[0] & 0xfc) >> 2;
            byte4[1] = ((byte3[0] & 0x03) << 4) | ((byte3[1] & 0xf0) >> 4);
            byte4[2] = ((byte3[1] & 0x0f) << 2) | ((byte3[2] & 0xc0) >> 6);
            byte4[3] = byte3[2] & 0x3f;

            // 将4个字符加入到输出中
            for (i = 0; i < 4; i++) {
                output[j++] = base64_chars[byte4[i]];
            }
            i = 0;
        }
    }

    // 如果还有剩余的字节不足3个，处理
    if (i != 0) {
        for (int k = i; k < 3; k++) {
            byte3[k] = 0;  // 填充 0
        }
        byte4[0] = (byte3[0] & 0xfc) >> 2;
        byte4[1] = ((byte3[0] & 0x03) << 4) | ((byte3[1] & 0xf0) >> 4);
        byte4[2] = ((byte3[1] & 0x0f) << 2) | ((byte3[2] & 0xc0) >> 6);
        // 将剩余的字符加入到输出
        for (int k = 0; k < i + 1; k++) {
            output[j++] = base64_chars[byte4[k]];
        }

        // 填充 '='
        while (i++ < 3) {
            output[j++] = '=';
        }
    }
    output[j] = '\0';  // 添加字符串终止符
    
    //清理data_length和eggs
    data_length = 0;
    memset(eggs, 0, MAXLENGTH * sizeof(struct data));
}
