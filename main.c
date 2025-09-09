
/* 掉电不丢失任务 */
static void PowerLossProtectionTask(void *pvParameters)
{
    // 设备启动时读取Flash中的数据以恢复状态
    uint8_t recoveryData[256]; // 假设存储的数据大小为256字节
    if (SPI_FLASH_Read(recoveryData, RECOVERADDR * SPI_FLASH_EraseSize, sizeof(recoveryData)) == 0)
    {
        // 恢复运行状态
        printf("恢复断电前数据成功\n");
        // TODO: 根据recoveryData恢复设备状态
    }
    else
    {
        printf("读取断电恢复数据失败\n");
    }

    while (1)
    {
        // 定期将关键数据写入Flash
        uint8_t currentData[256]; // 假设当前运行数据
        // TODO: 填充currentData为当前运行状态数据
        if (SPI_FLASH_Write(currentData, RECOVERADDR * SPI_FLASH_EraseSize, sizeof(currentData)) == 0)
        {
            printf("关键数据已保存到Flash\n");
        }
        else
        {
            printf("保存关键数据失败\n");
        }
        vTaskDelay(pdMS_TO_TICKS(60000)); // 每分钟保存一次
    }
}

int main(void)
{
    // 创建掉电不丢失任务
    xTaskCreate(PowerLossProtectionTask, "PowerLossProtection", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}
