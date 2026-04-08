#include "device_driver.h"

// 외부 상태 표시용 LED 6개 초기화 (PC6 ~ PC11)
void Status_LED_Init(void)
{
    // 1. GPIOC 클럭 켜기
    Macro_Set_Bit(RCC->AHB1ENR, 2); 

    // 2. PC6 ~ PC11 핀을 '출력(Output)' 모드로 설정
    for(int i = 6; i <= 11; i++) {
        Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, i * 2);
        Macro_Clear_Bit(GPIOC->OTYPER, i); // Push-Pull
        Macro_Write_Block(GPIOC->OSPEEDR, 0x3, 0x2, i * 2); // Fast speed
        Macro_Clear_Bit(GPIOC->ODR, i); // 초기 상태: 모두 끄기
    }
}

// 모든 LED 끄기
void Status_LED_All_Off(void)
{
    for(int i = 6; i <= 11; i++) {
        Macro_Clear_Bit(GPIOC->ODR, i);
    }
}

// 빨간색 2개 켜기 (나머지는 끔)
void Status_LED_Red(void)
{
    Status_LED_All_Off();
    Macro_Set_Bit(GPIOC->ODR, 6);
    Macro_Set_Bit(GPIOC->ODR, 7);
}

// 노란색 2개 켜기 (나머지는 끔)
void Status_LED_Yellow(void)
{
    Status_LED_All_Off();
    Macro_Set_Bit(GPIOC->ODR, 8);
    Macro_Set_Bit(GPIOC->ODR, 9);
}

// 초록색 2개 켜기 (나머지는 끔)
void Status_LED_Green(void)
{
    Status_LED_All_Off();
    Macro_Set_Bit(GPIOC->ODR, 10);
    Macro_Set_Bit(GPIOC->ODR, 11);
}