#include "./iwdg/iwdg.h"
void IWDG_Init(void)
{
    //解除写保护
	IWDG->KR = 0X5555;
	//预分频值（往PR寄存器写值）
	IWDG->PR = 0;               //4分频
	//重装载值（往RLR寄存器写值）
	IWDG->RLR = 5000;
	//启动看门狗
	IWDG->KR = 0XCCCC;
    //使能写保护
    IWDG->KR = 0X0000;
}
/*10ms调用一次即可*/
void IWDG_Feed(void)
{
    //解除写保护
    IWDG->KR = 0X5555;
    //喂狗
    IWDG->KR = 0XAAAA;
    //使能写保护
    IWDG->KR = 0X0000;
}
