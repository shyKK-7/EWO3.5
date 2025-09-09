#include "./delay/delay.h"
						  
//void Delay_Init(void)
//{
//    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
//}
//void Delay_us(u16 us)
//{
//    SysTick->VAL = 0;
//    SysTick->LOAD = Systick_us * us;
//    SysTick->CTRL = 0x05;
//    while(!(SysTick->CTRL & 0x00010000));
//    SysTick->CTRL = 0x04;
//}
//void Delay_ms(u16 ms)
//{
//    while(ms--)
//    {
//        Delay_us(1000);
//    }
//}
     
                          
                          
//延时nus
//nus:要延时的us数.	
//nus:0~204522252(最大值即2^32/fac_us@fac_us=168)	    								   
void Delay_us(u32 us)
{		
    uint32_t count;
    while (us--) {
        // 每个循环大约1微秒
        for (count = 0; count < 33; count++) {
            __asm__("nop"); // 使用nop指令确保每个循环迭代的准确性
        }
    }								    
}  
//延时nms
//nms:要延时的ms数
//nms:0~65535
void Delay_ms(u32 nms)
{	
	u32 i;
	for(i=0;i<nms;i++) Delay_us(1000);
}
 
//延时nms,不会引起任务调度
//nms:要延时的ms数
void Delay_xms(u32 nms)
{
	
	u32 i;
	for(i=0;i<nms;i++) Delay_us(1000);
	

}

//void Delay_us_lora(u16 us)
//{
//    SysTick->VAL = 0;
//    SysTick->LOAD = Systick_us * us;
//    SysTick->CTRL = 0x05;
//    while(!(SysTick->CTRL & 0x00010000));
//    SysTick->CTRL = 0x04;
//}
//void Delay_ms_lora(u16 ms)
//{
//    while(ms--)
//    {
//        Delay_us(1000);
//    }
//}
