#include "device_driver.h"

// 외부 상태 표시용 LED 6개 초기화 (PB12~15, PA7~8)
void Status_LED_Init(void)
{
    // 1. GPIOA, GPIOB 클럭 켜기 (0번 비트, 1번 비트)
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    Macro_Set_Bit(RCC->AHB1ENR, 1); 

    // 2. PB12, PB13, PB14, PB15 핀 '출력(Output)' 모드 설정
    for(int i = 12; i <= 15; i++) {
        Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, i * 2);
        Macro_Clear_Bit(GPIOB->OTYPER, i);                  // Push-Pull
        Macro_Write_Block(GPIOB->OSPEEDR, 0x3, 0x2, i * 2); // Fast speed
        Macro_Clear_Bit(GPIOB->ODR, i);                     // 초기 상태: 끄기
    }

    // 3. PA7, PA8 핀 '출력(Output)' 모드 설정
    for(int i = 7; i <= 8; i++) {
        Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, i * 2);
        Macro_Clear_Bit(GPIOA->OTYPER, i);                  // Push-Pull
        Macro_Write_Block(GPIOA->OSPEEDR, 0x3, 0x2, i * 2); // Fast speed
        Macro_Clear_Bit(GPIOA->ODR, i);                     // 초기 상태: 끄기
    }
}

// 모든 LED 끄기
void Status_LED_All_Off(void)
{
    // PB12 ~ PB15 끄기
    for(int i = 12; i <= 15; i++) {
        Macro_Clear_Bit(GPIOB->ODR, i);
    }
    // PA7, PA8 끄기
    Macro_Clear_Bit(GPIOA->ODR, 7);
    Macro_Clear_Bit(GPIOA->ODR, 8);
}

// 빨간색 2개 켜기 (PB12, PB13)
void Status_LED_Red(void)
{
    Status_LED_All_Off();
    Macro_Set_Bit(GPIOB->ODR, 12);
    Macro_Set_Bit(GPIOB->ODR, 13);
}

// 노란색 2개 켜기 (PB14, PB15)
void Status_LED_Yellow(void)
{
    Status_LED_All_Off();
    Macro_Set_Bit(GPIOB->ODR, 14);
    Macro_Set_Bit(GPIOB->ODR, 15);
}

// 초록색 2개 켜기 (PA7, PA8)
void Status_LED_Green(void)
{
    Status_LED_All_Off();
    Macro_Set_Bit(GPIOA->ODR, 7);
    Macro_Set_Bit(GPIOA->ODR, 8);
}