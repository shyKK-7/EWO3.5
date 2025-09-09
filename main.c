
/* ���粻��ʧ���� */
static void PowerLossProtectionTask(void *pvParameters)
{
    // �豸����ʱ��ȡFlash�е������Իָ�״̬
    uint8_t recoveryData[256]; // ����洢�����ݴ�СΪ256�ֽ�
    if (SPI_FLASH_Read(recoveryData, RECOVERADDR * SPI_FLASH_EraseSize, sizeof(recoveryData)) == 0)
    {
        // �ָ�����״̬
        printf("�ָ��ϵ�ǰ���ݳɹ�\n");
        // TODO: ����recoveryData�ָ��豸״̬
    }
    else
    {
        printf("��ȡ�ϵ�ָ�����ʧ��\n");
    }

    while (1)
    {
        // ���ڽ��ؼ�����д��Flash
        uint8_t currentData[256]; // ���赱ǰ��������
        // TODO: ���currentDataΪ��ǰ����״̬����
        if (SPI_FLASH_Write(currentData, RECOVERADDR * SPI_FLASH_EraseSize, sizeof(currentData)) == 0)
        {
            printf("�ؼ������ѱ��浽Flash\n");
        }
        else
        {
            printf("����ؼ�����ʧ��\n");
        }
        vTaskDelay(pdMS_TO_TICKS(60000)); // ÿ���ӱ���һ��
    }
}

int main(void)
{
    // �������粻��ʧ����
    xTaskCreate(PowerLossProtectionTask, "PowerLossProtection", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}
