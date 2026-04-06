#include "device_driver.h"

void Key_Poll_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 2); 
	Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);
}

int Key_Get_Pressed(void)
{
	return Macro_Check_Bit_Clear(GPIOC->IDR, 13);	
}

void Key_Wait_Key_Pressed(void)
{
	while(!Macro_Check_Bit_Clear(GPIOC->IDR, 13));
}

void Key_Wait_Key_Released(void)
{
	while(!Macro_Check_Bit_Set(GPIOC->IDR, 13));
}

void Key_ISR_Enable(int en)
{
	if(en)
	{
		Macro_Set_Bit(RCC->AHB1ENR, 2); 
		Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);

		// SYSCFG 장치 Clock On
		Macro_Set_Bit(RCC->APB2ENR, 14);
		// PC13을 EXTI 13의 소스가 되도록 설정
		Macro_Write_Block(SYSCFG->EXTICR[3], 0xF, 0x2, 4);
		// EXTI 13을 Falling Edge Trigger로 설정
		Macro_Set_Bit(EXTI->FTSR, 13);
		// EXTI 13 Pending Clear
		EXTI->PR = 0x1<<13;
		// Macro_Set_Bit(EXTI->PR, 13);			 	Pending Clear는 매크로를 사용해선 안되며 직접적인 대입만을 해야한다, 다른 Interrupt를 덮어 씌울 수도 있다
		// NVIC EXTI15_9 Interrupt Pending Clear
		NVIC_ClearPendingIRQ((IRQn_Type)40);
		// EXTI 13 Interrupt Enable
		Macro_Set_Bit(EXTI->IMR, 13);
		// NVIC EXTI15_9 Interrupt Enable
		NVIC_EnableIRQ((IRQn_Type)40);
	}

	else
	{
		NVIC_DisableIRQ((IRQn_Type)40);
	}
}
