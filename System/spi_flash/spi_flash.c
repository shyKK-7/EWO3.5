/**
  ******************************************************************************
  * @file    bsp_spi_flash.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   spi flash 底层应用函数bsp 

  */

#include "./spi_flash/spi_flash.h"

u32 save_cnt = 0; //已保存的数据个数

static __IO uint32_t SPITimeout = SPIT_LONG_TIMEOUT;

static u16 SPI_TIMEOUT_UserCallback(u8 errorCode);

/**
  * @brief  SPI_FLASH初始化
  * @param  无
  * @retval 无
  */
void SPI_FLASH_Init(void)
{
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能 FLASH_SPI 及GPIO 时钟 */
	/*!< SPI_FLASH_SPI_CS_GPIO, SPI_FLASH_SPI_MOSI_GPIO, 
       SPI_FLASH_SPI_MISO_GPIO,SPI_FLASH_SPI_SCK_GPIO 时钟使能 */
	RCC_AHB1PeriphClockCmd(FLASH_SPI_SCK_GPIO_CLK | FLASH_SPI_MISO_GPIO_CLK | FLASH_SPI_MOSI_GPIO_CLK | FLASH_CS_GPIO_CLK, ENABLE);

	/*!< SPI_FLASH_SPI 时钟使能 */
	FLASH_SPI_CLK_INIT(FLASH_SPI_CLK, ENABLE);

	//设置引脚复用
	GPIO_PinAFConfig(FLASH_SPI_SCK_GPIO_PORT, FLASH_SPI_SCK_PINSOURCE, FLASH_SPI_SCK_AF);
	GPIO_PinAFConfig(FLASH_SPI_MISO_GPIO_PORT, FLASH_SPI_MISO_PINSOURCE, FLASH_SPI_MISO_AF);
	GPIO_PinAFConfig(FLASH_SPI_MOSI_GPIO_PORT, FLASH_SPI_MOSI_PINSOURCE, FLASH_SPI_MOSI_AF);

	/*!< 配置 SPI_FLASH_SPI 引脚: SCK */
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(FLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

	/*!< 配置 SPI_FLASH_SPI 引脚: MISO */
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MISO_PIN;
	GPIO_Init(FLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

	/*!< 配置 SPI_FLASH_SPI 引脚: MOSI */
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MOSI_PIN;
	GPIO_Init(FLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

	/*!< 配置 SPI_FLASH_SPI 引脚: CS */
	GPIO_InitStructure.GPIO_Pin = FLASH_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(FLASH_CS_GPIO_PORT, &GPIO_InitStructure);

	/* 停止信号 FLASH: CS引脚高电平*/
	SPI_FLASH_CS_HIGH();

	/* FLASH_SPI 模式配置 */
	// FLASH芯片 支持SPI模式0及模式3，据此设置CPOL CPHA
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(FLASH_SPI, &SPI_InitStructure);

	/* 使能 FLASH_SPI  */
	SPI_Cmd(FLASH_SPI, ENABLE);
    
    printf("FLASH INIT SUCCESS, ID: %X\n", SPI_FLASH_ReadID());
}

/**
  * @brief  擦除FLASH扇区
  * @param  SectorAddr：要擦除的扇区地址
  * @retval 无
  */
void SPI_FLASH_SectorErase(u32 SectorAddr)
{

    /* 发送FLASH写使能命令 */
    SPI_FLASH_WriteEnable();
    SPI_FLASH_WaitForWriteEnd();
    /* 擦除扇区 */
    /* 选择FLASH: CS低电平 */
    SPI_FLASH_CS_LOW();
    /* 发送扇区擦除指令*/
    SPI_FLASH_SendByte(W25X_SectorErase);
    /*发送擦除扇区地址的高位*/
    SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
    /* 发送擦除扇区地址的中位 */
    SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);
    /* 发送擦除扇区地址的低位 */
    SPI_FLASH_SendByte(SectorAddr & 0xFF);
    /* 停止信号 FLASH: CS 高电平 */
    SPI_FLASH_CS_HIGH();
    /* 等待擦除完毕*/
    SPI_FLASH_WaitForWriteEnd();
}
void SPI_FLASH_Erase(u32 SectorAddr, u16 size)
{
    u32 addr = SectorAddr;
    u8 i;

    // 计算需要擦除的扇区数量，向上取整
    i = (size + SPI_FLASH_EraseSize - 1) / SPI_FLASH_EraseSize;

    while(i--)
    {
        // 执行单次擦除
        SPI_FLASH_SectorErase(addr);
        addr += SPI_FLASH_EraseSize;  // 更新到下一个扇区地址
    }
}

void SPI_FLASH_SectorErase_NOS(u32 SectorAddr)
{
    /* 发送FLASH写使能命令 */
    SPI_FLASH_WriteEnable();
    SPI_FLASH_WaitForWriteEnd();
    /* 擦除扇区 */
    /* 选择FLASH: CS低电平 */
    SPI_FLASH_CS_LOW();
    /* 发送扇区擦除指令*/
    SPI_FLASH_SendByte(W25X_SectorErase);
    /*发送擦除扇区地址的高位*/
    SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
    /* 发送擦除扇区地址的中位 */
    SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);
    /* 发送擦除扇区地址的低位 */
    SPI_FLASH_SendByte(SectorAddr & 0xFF);
    /* 停止信号 FLASH: CS 高电平 */
    SPI_FLASH_CS_HIGH();
    /* 等待擦除完毕*/
    SPI_FLASH_WaitForWriteEnd();
}
/**
  * @brief  擦除FLASH扇区，整片擦除
  * @param  无
  * @retval 无
  */
void SPI_FLASH_BulkErase(void)
{
    /* 发送FLASH写使能命令 */
    SPI_FLASH_WriteEnable();

    /* 整块 Erase */
    /* 选择FLASH: CS低电平 */
    SPI_FLASH_CS_LOW();
    /* 发送整块擦除指令*/
    SPI_FLASH_SendByte(W25X_ChipErase);
    /* 停止信号 FLASH: CS 高电平 */
    SPI_FLASH_CS_HIGH();

    /* 等待擦除完毕*/
    SPI_FLASH_WaitForWriteEnd();
}

/**
  * @brief  对FLASH按页写入数据，调用本函数写入数据前需要先擦除扇区
  * @param	pBuffer，要写入数据的指针
  * @param WriteAddr，写入地址
  * @param  NumByteToWrite，写入数据长度，必须小于等于SPI_FLASH_PerWritePageSize
  * @retval 无
  */
void SPI_FLASH_PageWrite(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
		/* 发送FLASH写使能命令 */
		SPI_FLASH_WriteEnable();

		/* 选择FLASH: CS低电平 */
		SPI_FLASH_CS_LOW();
		/* 写页写指令*/
		SPI_FLASH_SendByte(W25X_PageProgram);
		/*发送写地址的高位*/
		SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
		/*发送写地址的中位*/
		SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);
		/*发送写地址的低位*/
		SPI_FLASH_SendByte(WriteAddr & 0xFF);

		if (NumByteToWrite > SPI_FLASH_PerWritePageSize)
		{
			NumByteToWrite = SPI_FLASH_PerWritePageSize;
			FLASH_ERROR("SPI_FLASH_PageWrite too large!");
		}

		/* 写入数据*/
		while (NumByteToWrite--)
		{
			/* 发送当前要写入的字节数据 */
			SPI_FLASH_SendByte(*pBuffer);
			/* 指向下一字节数据 */
			pBuffer++;
		}

		/* 停止信号 FLASH: CS 高电平 */
		SPI_FLASH_CS_HIGH();

		/* 等待写入完毕*/
		SPI_FLASH_WaitForWriteEnd();
}

/**
  * @brief  对FLASH写入数据，调用本函数写入数据前需要先擦除扇区
  * @param	pBuffer，要写入数据的指针
  * @param  WriteAddr，写入地址
  * @param  NumByteToWrite，写入数据长度
  * @retval 无
  */
void SPI_FLASH_BufferWrite(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
    /*mod运算求余，若writeAddr是SPI_FLASH_PageSize整数倍，运算结果Addr值为0*/
    Addr = WriteAddr % SPI_FLASH_PageSize;

    /*差count个数据值，刚好可以对齐到页地址*/
    count = SPI_FLASH_PageSize - Addr;
    /*计算出要写多少整数页*/
    NumOfPage = NumByteToWrite / SPI_FLASH_PageSize;
    /*mod运算求余，计算出剩余不满一页的字节数*/
    NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

    /* Addr=0,则WriteAddr 刚好按页对齐 aligned  */
    if (Addr == 0)
    {
        /* NumByteToWrite < SPI_FLASH_PageSize */
        if (NumOfPage == 0)
        {
            SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
        }
        else /* NumByteToWrite > SPI_FLASH_PageSize */
        {
            /*先把整数页都写了*/
            while (NumOfPage--)
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
                WriteAddr += SPI_FLASH_PageSize;
                pBuffer += SPI_FLASH_PageSize;
            }

            /*若有多余的不满一页的数据，把它写完*/
            SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        }
    }
    /* 若地址与 SPI_FLASH_PageSize 不对齐  */
    else
    {
        /* NumByteToWrite < SPI_FLASH_PageSize */
        if (NumOfPage == 0)
        {
            /*当前页剩余的count个位置比NumOfSingle小，写不完*/
            if (NumOfSingle > count)
            {
                temp = NumOfSingle - count;

                /*先写满当前页*/
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
                WriteAddr += count;
                pBuffer += count;

                /*再写剩余的数据*/
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, temp);
            }
            else /*当前页剩余的count个位置能写完NumOfSingle个数据*/
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
            }
        }
        else /* NumByteToWrite > SPI_FLASH_PageSize */
        {
            /*地址不对齐多出的count分开处理，不加入这个运算*/
            NumByteToWrite -= count;
            NumOfPage = NumByteToWrite / SPI_FLASH_PageSize;
            NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;

            SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
            WriteAddr += count;
            pBuffer += count;

            /*把整数页都写了*/
            while (NumOfPage--)
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
                WriteAddr += SPI_FLASH_PageSize;
                pBuffer += SPI_FLASH_PageSize;
            }
            /*若有多余的不满一页的数据，把它写完*/
            if (NumOfSingle != 0)
            {
                SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
            }
        }
    }
}

/**
  * @brief  读取FLASH数据
  * @param 	pBuffer，存储读出数据的指针
  * @param   ReadAddr，读取地址
  * @param   NumByteToRead，读取数据长度
  * @retval 无
  */
void SPI_FLASH_BufferRead(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
    /* 选择FLASH: CS低电平 */
    SPI_FLASH_CS_LOW();

    /* 发送 读 指令 */
    SPI_FLASH_SendByte(W25X_ReadData);

    /* 发送 读 地址高位 */
    SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
    /* 发送 读 地址中位 */
    SPI_FLASH_SendByte((ReadAddr & 0xFF00) >> 8);
    /* 发送 读 地址低位 */
    SPI_FLASH_SendByte(ReadAddr & 0xFF);

    /* 读取数据 */
    while (NumByteToRead--)
    {
        /* 读取一个字节*/
        *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);
        /* 指向下一个字节缓冲区 */
        pBuffer++;
    }

    /* 停止信号 FLASH: CS 高电平 */
    SPI_FLASH_CS_HIGH();
}



/**
  * @brief  读取FLASH ID
  * @param 	无
  * @retval FLASH ID
  */
u32 SPI_FLASH_ReadID(void)
{
	u32 Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
    /* 开始通讯：CS低电平 */
    SPI_FLASH_CS_LOW();

    /* 发送JEDEC指令，读取ID */
    SPI_FLASH_SendByte(W25X_JedecDeviceID);

    /* 读取一个字节数据 */
    Temp0 = SPI_FLASH_SendByte(Dummy_Byte);

    /* 读取一个字节数据 */
    Temp1 = SPI_FLASH_SendByte(Dummy_Byte);

    /* 读取一个字节数据 */
    Temp2 = SPI_FLASH_SendByte(Dummy_Byte);

    /* 停止通讯：CS高电平 */
    SPI_FLASH_CS_HIGH();

    /*把数据组合起来，作为函数的返回值*/
    Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
	return Temp;
}

/**
  * @brief  读取FLASH Device ID
  * @param 	无
  * @retval FLASH Device ID
  */
u32 SPI_FLASH_ReadDeviceID(void)
{
	u32 Temp = 0;
    /* Select the FLASH: Chip Select low */
    SPI_FLASH_CS_LOW();

    /* Send "RDID " instruction */
    SPI_FLASH_SendByte(W25X_DeviceID);
    SPI_FLASH_SendByte(Dummy_Byte);
    SPI_FLASH_SendByte(Dummy_Byte);
    SPI_FLASH_SendByte(Dummy_Byte);

    /* Read a byte from the FLASH */
    Temp = SPI_FLASH_SendByte(Dummy_Byte);

    /* Deselect the FLASH: Chip Select high */
    SPI_FLASH_CS_HIGH();

	return Temp;
}
/*******************************************************************************
* Function Name  : SPI_FLASH_StartReadSequence
* Description    : Initiates a read data byte (READ) sequence from the Flash.
*                  This is done by driving the /CS line low to select the device,
*                  then the READ instruction is transmitted followed by 3 bytes
*                  address. This function exit and keep the /CS line low, so the
*                  Flash still being selected. With this technique the whole
*                  content of the Flash is read with a single READ instruction.
* Input          : - ReadAddr : FLASH's internal address to read from.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_StartReadSequence(u32 ReadAddr)
{
	/* Select the FLASH: Chip Select low */
	SPI_FLASH_CS_LOW();

	/* Send "Read from Memory " instruction */
	SPI_FLASH_SendByte(W25X_ReadData);

	/* Send the 24-bit address of the address to read from -----------------------*/
	/* Send ReadAddr high nibble address byte */
	SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
	/* Send ReadAddr medium nibble address byte */
	SPI_FLASH_SendByte((ReadAddr & 0xFF00) >> 8);
	/* Send ReadAddr low nibble address byte */
	SPI_FLASH_SendByte(ReadAddr & 0xFF);
}

/**
  * @brief  使用SPI读取一个字节的数据
  * @param  无
  * @retval 返回接收到的数据
  */
u8 SPI_FLASH_ReadByte(void)
{
	return (SPI_FLASH_SendByte(Dummy_Byte));
}

/**
  * @brief  使用SPI发送一个字节的数据
  * @param  byte：要发送的数据
  * @retval 返回接收到的数据
  */
u8 SPI_FLASH_SendByte(u8 byte)
{
	SPITimeout = SPIT_FLAG_TIMEOUT;

	/* 等待发送缓冲区为空，TXE事件 */
	while (SPI_I2S_GetFlagStatus(FLASH_SPI, SPI_I2S_FLAG_TXE) == RESET)
	{
		if ((SPITimeout--) == 0)
			return SPI_TIMEOUT_UserCallback(0);
	}

	/* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
	SPI_I2S_SendData(FLASH_SPI, byte);

	SPITimeout = SPIT_FLAG_TIMEOUT;

	/* 等待接收缓冲区非空，RXNE事件 */
	while (SPI_I2S_GetFlagStatus(FLASH_SPI, SPI_I2S_FLAG_RXNE) == RESET)
	{
		if ((SPITimeout--) == 0)
			return SPI_TIMEOUT_UserCallback(1);
	}

	/* 读取数据寄存器，获取接收缓冲区数据 */
	return SPI_I2S_ReceiveData(FLASH_SPI);
}

/*******************************************************************************
* Function Name  : SPI_FLASH_SendHalfWord
* Description    : Sends a Half Word through the SPI interface and return the
*                  Half Word received from the SPI bus.
* Input          : Half Word : Half Word to send.
* Output         : None
* Return         : The value of the received Half Word.
*******************************************************************************/
u16 SPI_FLASH_SendHalfWord(u16 HalfWord)
{

	SPITimeout = SPIT_FLAG_TIMEOUT;

	/* Loop while DR register in not emplty */
	while (SPI_I2S_GetFlagStatus(FLASH_SPI, SPI_I2S_FLAG_TXE) == RESET)
	{
		if ((SPITimeout--) == 0)
			return SPI_TIMEOUT_UserCallback(2);
	}

	/* Send Half Word through the FLASH_SPI peripheral */
	SPI_I2S_SendData(FLASH_SPI, HalfWord);

	SPITimeout = SPIT_FLAG_TIMEOUT;

	/* Wait to receive a Half Word */
	while (SPI_I2S_GetFlagStatus(FLASH_SPI, SPI_I2S_FLAG_RXNE) == RESET)
	{
		if ((SPITimeout--) == 0)
			return SPI_TIMEOUT_UserCallback(3);
	}
	/* Return the Half Word read from the SPI bus */
	return SPI_I2S_ReceiveData(FLASH_SPI);
}

/**
  * @brief  向FLASH发送 写使能 命令
  * @param  none
  * @retval none
  */
void SPI_FLASH_WriteEnable(void)
{
	/* 通讯开始：CS低 */
	SPI_FLASH_CS_LOW();

	/* 发送写使能命令*/
	SPI_FLASH_SendByte(W25X_WriteEnable);

	/*通讯结束：CS高 */
	SPI_FLASH_CS_HIGH();
}

/**
  * @brief  等待WIP(BUSY)标志被置0，即等待到FLASH内部数据写入完毕
  * @param  none
  * @retval none
  */
void SPI_FLASH_WaitForWriteEnd(void)
{
	u8 FLASH_Status = 0;

	/* 选择 FLASH: CS 低 */
	SPI_FLASH_CS_LOW();

	/* 发送 读状态寄存器 命令 */
	SPI_FLASH_SendByte(W25X_ReadStatusReg);

	SPITimeout = SPIT_FLAG_TIMEOUT;
	/* 若FLASH忙碌，则等待 */
	do
	{
		/* 读取FLASH芯片的状态寄存器 */
		FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);

		{
			if ((SPITimeout--) == 0)
			{
				SPI_TIMEOUT_UserCallback(4);
				return;
			}
		}
	} while ((FLASH_Status & WIP_Flag) == SET); /* 正在写入标志 */

	/* 停止信号  FLASH: CS 高 */
	SPI_FLASH_CS_HIGH();
}

//进入掉电模式
void SPI_Flash_PowerDown(void)
{
	/* 选择 FLASH: CS 低 */
	SPI_FLASH_CS_LOW();

	/* 发送 掉电 命令 */
	SPI_FLASH_SendByte(W25X_PowerDown);

	/* 停止信号  FLASH: CS 高 */
	SPI_FLASH_CS_HIGH();
}

//唤醒
void SPI_Flash_WAKEUP(void)
{
	/*选择 FLASH: CS 低 */
	SPI_FLASH_CS_LOW();

	/* 发上 上电 命令 */
	SPI_FLASH_SendByte(W25X_ReleasePowerDown);

	/* 停止信号 FLASH: CS 高 */
	SPI_FLASH_CS_HIGH(); //等待TRES1
}

/**
  * @brief  等待超时回调函数
  * @param  None.
  * @retval None.
  */
static u16 SPI_TIMEOUT_UserCallback(u8 errorCode)
{
	/* 等待超时后的处理,输出错误信息 */
	FLASH_ERROR("SPI 等待超时!errorCode = %d", errorCode);
	return 0;
}


/*********************************************END OF FILE**********************/




/***********************************鹅的队列信息********************************/


//校验标志位
u8 Egg_Queue_Flag = 0; 


// 蛋队列数组定义（全局变量）
Egg_Queue_Array egg_queue_array;

//初始化蛋队列：从 Flash 加载数据
void Egg_Queue_Init(void) {
		if(Egg_Queue_Exists())
		{
			Egg_Queue_Load(); 
			Egg_Queue_Flag = 1;
			FLASH_INFO("Egg queue loaded from Flash OK_OK");
		}
		else
		{
			Egg_Queue_Flag = 0;
			FLASH_INFO("No egg queue in Flash, initialized empty");
		}
	
}

//将蛋队列保存到 Flash
void Egg_Queue_Save(void) {
    u8 buffer[sizeof(Egg_Queue_Array)];
    u32 start_address = SECTOR(EGG_QUEUE_STARTADDR);

    // 将结构体复制为字节流
    memcpy(buffer, &egg_queue_array, sizeof(Egg_Queue_Array));

    // 计算所需扇区数量
    int total_size = sizeof(Egg_Queue_Array);
    int sectors_needed = (total_size + SPI_FLASH_EraseSize - 1) / SPI_FLASH_EraseSize;
    int allocated_sectors = EGG_QUEUE_ENDADDR - EGG_QUEUE_STARTADDR + 1;

    if (sectors_needed > allocated_sectors) {
        FLASH_ERROR("Egg queue too large for allocated Flash sectors! Need:%d, Available:%d", 
                    sectors_needed, allocated_sectors);
        return;
    }

    // 擦除扇区
    for (int i = 0; i < sectors_needed; i++) {
        u32 sector_addr = start_address + i * SPI_FLASH_EraseSize;
        SPI_FLASH_SectorErase(sector_addr);
        FLASH_INFO("Erased sector at 0x%X for egg queue", sector_addr);
    }

    // 分块写入
    u32 write_addr = start_address;
    u32 remaining = total_size;
    u32 offset = 0;

    while (remaining > 0) {
        u32 write_size = (remaining > SPI_FLASH_EraseSize) ? SPI_FLASH_EraseSize : remaining;
        SPI_FLASH_BufferWrite(buffer + offset, write_addr, write_size);
        FLASH_INFO("Wrote %u bytes to 0x%X", write_size, write_addr);
        write_addr += SPI_FLASH_EraseSize;
        offset += write_size;
        remaining -= write_size;
    }

    FLASH_INFO("Egg queue saved, %d sectors used, addr=0x%X", sectors_needed, start_address);
}

//从 Flash 加载蛋队列
void Egg_Queue_Load(void) {
    u8 buffer[sizeof(Egg_Queue_Array)];
    u32 start_address = SECTOR(EGG_QUEUE_STARTADDR);
    u32 total_size = sizeof(Egg_Queue_Array);
    u32 read_addr = start_address;
    u32 remaining = total_size;
    u32 offset = 0;

    // 读取所有数据
    while (remaining > 0 && read_addr <= SECTOR(EGG_QUEUE_ENDADDR)) {
        u32 read_size = (remaining > SPI_FLASH_EraseSize) ? SPI_FLASH_EraseSize : remaining;
        SPI_FLASH_BufferRead(buffer + offset, read_addr, read_size);
        FLASH_INFO("Read %u bytes from 0x%X", read_size, read_addr);
        read_addr += SPI_FLASH_EraseSize;
        offset += read_size;
        remaining -= read_size;
    }

    // 复制到内存结构
    memcpy(&egg_queue_array, buffer, sizeof(Egg_Queue_Array));

    // 校验数据有效性
    if (egg_queue_array.Egg_queue_length < 0 || egg_queue_array.Egg_queue_length > MAX_EGG_COUNT) {
        Egg_Queue_Clear();  // 无效则清空
        FLASH_INFO("Invalid egg queue length (%d), initialized to empty", egg_queue_array.Egg_queue_length);
    } else {
        FLASH_INFO("Egg queue loaded, length: %d", egg_queue_array.Egg_queue_length);
    }
}

//添加一个蛋到队列
void Egg_Queue_Add(const Egg_Queue *egg) {
    if (egg == NULL) {
        FLASH_ERROR("Attempt to add NULL egg to queue");
        return;
    }

    if (egg_queue_array.Egg_queue_length >= MAX_EGG_COUNT) {
        FLASH_ERROR("Egg queue is full, cannot add new egg (current: %d)", egg_queue_array.Egg_queue_length);
        return;
    }

    // 获取插入位置
    Egg_Queue *dest = &egg_queue_array.Egg_Queue_data[egg_queue_array.Egg_queue_length];
    memcpy(dest, egg, sizeof(Egg_Queue));
    egg_queue_array.Egg_queue_length++;

    // 保存到 Flash
    Egg_Queue_Save();
    
    // 日志：更新打印信息，包含入窝/出窝时间
    FLASH_INFO("Egg added, ID: %02X%02X..., in:%u, out:%u, goose_cnt:%u, egg_ts:%u, new_len:%d",
               egg->id[0], egg->id[1], 
               egg->egg_intime, 
               egg->egg_outtime, 
               egg->goose_count, 
               egg->egg_timestamp, 
               egg_queue_array.Egg_queue_length);
}

//清空内存中的蛋队列（不擦除 Flash）
void Egg_Queue_Clear(void) {
    egg_queue_array.Egg_queue_length = 0;
    memset(egg_queue_array.Egg_Queue_data, 0, sizeof(egg_queue_array.Egg_Queue_data));
    FLASH_INFO("Egg queue cleared in RAM");
}

//重置蛋队列：擦除 Flash 并清空内存
void Egg_Queue_Reset(void) {
    u32 start_address = SECTOR(EGG_QUEUE_STARTADDR);
    int sector_count = EGG_QUEUE_ENDADDR - EGG_QUEUE_STARTADDR + 1;

    // 擦除所有分配的扇区
    for (int i = 0; i < sector_count; i++) {
        u32 addr = start_address + i * SPI_FLASH_EraseSize;
        SPI_FLASH_SectorErase(addr);
        FLASH_INFO("Erased sector 0x%X for egg queue reset", addr);
    }

    // 内存清零
    Egg_Queue_Clear();

    // 可选：写入空队列，确保 Flash 状态一致
    Egg_Queue_Save();

    FLASH_INFO("Egg queue reset in Flash and RAM");
}

//判断 Flash 中是否存在有效的蛋队列数据
bool Egg_Queue_Exists(void) {
    u32 start_address = SECTOR(EGG_QUEUE_STARTADDR);
    u8 first_byte;

    // 读取第一个字节
    SPI_FLASH_BufferRead(&first_byte, start_address, 1);

    // 如果是 0xFF，说明没写过（空的）
    if (first_byte == 0xFF) {
        return false;
    } else {
        return true;
    }
}
