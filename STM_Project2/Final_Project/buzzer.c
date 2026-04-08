#include "device_driver.h"

void Buzzer_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 1);                         // GPIOB clock enable
    Macro_Write_Block(GPIOB->MODER,   0x3, 0x1, 0 * 2);    // PB0 output mode
    Macro_Clear_Bit(GPIOB->OTYPER, 0);                     // push-pull
    Macro_Write_Block(GPIOB->OSPEEDR, 0x3, 0x2, 0 * 2);    // medium/high speed
    Macro_Write_Block(GPIOB->PUPDR,   0x3, 0x0, 0 * 2);    // no pull
    Macro_Clear_Bit(GPIOB->ODR, 0);                        // buzzer off
}

void Buzzer_On(void)
{
    Macro_Set_Bit(GPIOB->ODR, 0);   // PB0 = High
}

void Buzzer_Off(void)
{
    Macro_Clear_Bit(GPIOB->ODR, 0); // PB0 = Low
}
