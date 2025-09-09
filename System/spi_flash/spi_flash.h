#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

#include "stm32f4xx.h" // Device header
#include <stdio.h>
#include <stdlib.h>
#include "./Struct/struct.h"
#include "easyflash.h"
/* 存放区块划分: 一共512块扇区，块0用来存放装置的运行实时状态，方便断电恢复，块1用来存放产蛋记录个数，也就是save_cnt, 块2用来存放显示屏校准参数，块3用来存放称校准参数，, 块4-块511都是存放产蛋记录*/

/* Private typedef -----------------------------------------------------------*/
//#define  sFLASH_ID                       0xEF3015     //W25X16
//#define  sFLASH_ID                       0xEF4015	    //W25Q16
#define sFLASH_ID 0XEF4017 //W25Q64
//#define  sFLASH_ID                       0XEF4018     //W25Q128

#define SPI_FLASH_EraseSize            4096
#define SPI_FLASH_PageSize             256
#define SPI_FLASH_PerWritePageSize     256    


#define FLASH_REREAD_TIMES   10         //当CRC校验出错时的重读次数
#define RECOVERADDR           0       //设备运行状态存储扇区         
#define SAVEADDR              1        // 产蛋记录个数存放扇区      
#define STARTADDR             5         //蛋信息的存储起始位置
#define ENDADDR               511       //蛋信息的存储结束位置
#define LENGTH_SUYUAN         20        //溯源的数据数量

#define EGG_QUEUE_STARTADDR   5        // 蛋队列信息存储起始扇区
#define EGG_QUEUE_ENDADDR     20       // 蛋队列信息存储结束扇区


/*块的首地址*/
#define BLOCK_BASE 0x000000
#define BLOCK_SIZE 0x010000
#define SECTOR_SIZE 0X001000

#define BLOCK(n) (BLOCK_BASE + (n)*BLOCK_SIZE)
#define SECTOR(n) (BLOCK_BASE + (n)*SECTOR_SIZE)
/* Private define ------------------------------------------------------------*/
/*命令定义-开头*******************************/
#define W25X_WriteEnable 0x06
#define W25X_WriteDisable 0x04
#define W25X_ReadStatusReg 0x05
#define W25X_WriteStatusReg 0x01
#define W25X_ReadData 0x03
#define W25X_FastReadData 0x0B
#define W25X_FastReadDual 0x3B
#define W25X_PageProgram 0x02
#define W25X_BlockErase 0xD8
#define W25X_SectorErase 0x20
#define W25X_ChipErase 0xC7
#define W25X_PowerDown 0xB9
#define W25X_ReleasePowerDown 0xAB
#define W25X_DeviceID 0xAB
#define W25X_ManufactDeviceID 0x90
#define W25X_JedecDeviceID 0x9F

#define WIP_Flag 0x01 /* Write In Progress (WIP) flag */
#define Dummy_Byte 0xFF
/*命令定义-结尾*******************************/



/*SPI接口定义-开头****************************/
#define FLASH_SPI SPI2
#define FLASH_SPI_CLK RCC_APB1Periph_SPI2
#define FLASH_SPI_CLK_INIT RCC_APB1PeriphClockCmd

///*开发板测试接口*/
//#define FLASH_SPI_SCK_PIN                   GPIO_Pin_5
//#define FLASH_SPI_SCK_GPIO_PORT             GPIOA
//#define FLASH_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOA
//#define FLASH_SPI_SCK_PINSOURCE             GPIO_PinSource5
//#define FLASH_SPI_SCK_AF                    GPIO_AF_SPI1

//#define FLASH_SPI_MISO_PIN                  GPIO_Pin_6
//#define FLASH_SPI_MISO_GPIO_PORT            GPIOA
//#define FLASH_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOA
//#define FLASH_SPI_MISO_PINSOURCE            GPIO_PinSource6
//#define FLASH_SPI_MISO_AF                   GPIO_AF_SPI1

//#define FLASH_SPI_MOSI_PIN                  GPIO_Pin_7
//#define FLASH_SPI_MOSI_GPIO_PORT            GPIOA
//#define FLASH_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOA
//#define FLASH_SPI_MOSI_PINSOURCE            GPIO_PinSource7
//#define FLASH_SPI_MOSI_AF                   GPIO_AF_SPI1

//#define FLASH_CS_PIN                        GPIO_Pin_0
//#define FLASH_CS_GPIO_PORT                  GPIOB
//#define FLASH_CS_GPIO_CLK                   RCC_AHB1Periph_GPIOB

#define FLASH_SPI_SCK_PIN GPIO_Pin_13
#define FLASH_SPI_SCK_GPIO_PORT GPIOB
#define FLASH_SPI_SCK_GPIO_CLK RCC_AHB1Periph_GPIOB
#define FLASH_SPI_SCK_PINSOURCE GPIO_PinSource13
#define FLASH_SPI_SCK_AF GPIO_AF_SPI2

#define FLASH_SPI_MISO_PIN GPIO_Pin_14
#define FLASH_SPI_MISO_GPIO_PORT GPIOB
#define FLASH_SPI_MISO_GPIO_CLK RCC_AHB1Periph_GPIOB
#define FLASH_SPI_MISO_PINSOURCE GPIO_PinSource14
#define FLASH_SPI_MISO_AF GPIO_AF_SPI2

#define FLASH_SPI_MOSI_PIN GPIO_Pin_15
#define FLASH_SPI_MOSI_GPIO_PORT GPIOB
#define FLASH_SPI_MOSI_GPIO_CLK RCC_AHB1Periph_GPIOB
#define FLASH_SPI_MOSI_PINSOURCE GPIO_PinSource15
#define FLASH_SPI_MOSI_AF GPIO_AF_SPI2

#define FLASH_CS_PIN GPIO_Pin_0
#define FLASH_CS_GPIO_PORT GPIOB
#define FLASH_CS_GPIO_CLK RCC_AHB1Periph_GPIOB

#define SPI_FLASH_CS_LOW() GPIO_WriteBit(FLASH_CS_GPIO_PORT, FLASH_CS_PIN, 0);
#define SPI_FLASH_CS_HIGH() GPIO_WriteBit(FLASH_CS_GPIO_PORT, FLASH_CS_PIN, 1);
/*SPI接口定义-结尾****************************/

/*等待超时时间*/
#define SPIT_FLAG_TIMEOUT ((uint32_t)0x1000)
#define SPIT_LONG_TIMEOUT ((uint32_t)(10 * SPIT_FLAG_TIMEOUT))

/*信息输出*/
#define FLASH_DEBUG_ON 1

#define FLASH_INFO(fmt, arg...) printf("<<-FLASH-INFO->> " fmt "\n", ##arg)
#define FLASH_ERROR(fmt, arg...) printf("<<-FLASH-ERROR->> " fmt "\n", ##arg)
#define FLASH_DEBUG(fmt, arg...)                                        \
    do                                                                  \
    {                                                                   \
        if (FLASH_DEBUG_ON)                                             \
            printf("<<-FLASH-DEBUG->> [%d]" fmt "\n", __LINE__, ##arg); \
    } while (0)



extern u32 save_cnt;

void SPI_FLASH_Init(void);
void SPI_FLASH_SectorErase(u32 SectorAddr);
void SPI_FLASH_Erase(u32 SectorAddr, u16 size);
void SPI_FLASH_BulkErase(void);
void SPI_FLASH_PageWrite(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void SPI_FLASH_BufferWrite(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void SPI_FLASH_BufferRead(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
u32 SPI_FLASH_ReadID(void);
u32 SPI_FLASH_ReadDeviceID(void);
void SPI_FLASH_StartReadSequence(u32 ReadAddr);
void SPI_Flash_PowerDown(void);
void SPI_Flash_WAKEUP(void);

u8 SPI_FLASH_ReadByte(void);
u8 SPI_FLASH_SendByte(u8 byte);
u16 SPI_FLASH_SendHalfWord(u16 HalfWord);
void SPI_FLASH_WriteEnable(void);
void SPI_FLASH_WaitForWriteEnd(void);



#define MAX_EGG_COUNT		 100


typedef struct {
		u8 id[12];
		u32 egg_intime;             // 入窝时间
    u32 egg_outtime;            // 出窝时间
		u32 goose_count;             // 产蛋时鹅的数量
    u32 egg_timestamp;           // 产蛋时间戳
} Egg_Queue;

typedef struct {
		Egg_Queue Egg_Queue_data[MAX_EGG_COUNT];
		int32_t Egg_queue_length;
} Egg_Queue_Array;

extern Egg_Queue_Array egg_queue_array;
extern u8 Egg_Queue_Flag; //Flash标志位

//蛋函数声明
void Egg_Queue_Init(void);
void Egg_Queue_Save(void);
void Egg_Queue_Load(void);
void Egg_Queue_Add(const Egg_Queue *egg);
void Egg_Queue_Clear(void);
void Egg_Queue_Reset(void);// 清空Flash中的历史数据
bool Egg_Queue_Exists(void);

#endif /* __SPI_FLASH_H */
