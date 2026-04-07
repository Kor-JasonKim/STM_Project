#include "device_driver.h"

void Ultrasonic_Init(void)
{
    // 1. GPIOC 클럭 활성화 (포트 C 사용)
    Macro_Set_Bit(RCC->AHB1ENR, 2);

    // 2. PC4 (Trig) -> 출력 모드 (01)
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 8);
    Macro_Clear_Bit(GPIOC->OTYPER, 4); // Push-pull
    Macro_Write_Block(GPIOC->OSPEEDR, 0x3, 0x3, 8); // High speed

    // 3. PC5 (Echo) -> 입력 모드 (00)
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 10);
    // [중요] Echo 핀이 공중에 떠서 노이즈를 타지 않게 Pull-down(10) 설정 적용
    Macro_Write_Block(GPIOC->PUPDR, 0x3, 0x2, 10); 

    // 초기 상태: Trig 핀 LOW
    Macro_Clear_Bit(GPIOC->ODR, 4);
}

int Ultrasonic_Get_Distance(void)
{
    volatile int timeout; // 최적화 방지
    int time = 0;

    // 1. Trig 핀에 확실하고 넉넉하게 High 신호 발사 (최소 10us 이상)
    Macro_Set_Bit(GPIOC->ODR, 4);
    for(volatile int i = 0; i < 5000; i++); // 96MHz 기준 넉넉한 딜레이
    Macro_Clear_Bit(GPIOC->ODR, 4);

    // 2. Echo 핀이 High가 될 때까지 대기
    timeout = 2000000; // 타임아웃 시간 20배 증가! (보드가 너무 빨라서 늘림)
    while(Macro_Check_Bit_Clear(GPIOC->IDR, 5))
    {
        if(--timeout == 0) return -1; // 센서 무응답 에러
    }

    // 3. Echo 핀이 High로 유지되는 시간 측정
    timeout = 2000000;
    while(Macro_Check_Bit_Set(GPIOC->IDR, 5))
    {
        time++;
        // 시간을 세는 루프 안에도 미세 딜레이를 주어 안정화
        for(volatile int i = 0; i < 15; i++); 
        if(--timeout == 0) return -2; // 거리가 너무 멂 (측정 범위 초과)
    }

    // 4. 거리 계산 (이 숫자는 테스트 후 조정이 필요할 수 있습니다)
    // 값이 너무 크게 나오면 100을 200으로, 작게 나오면 50으로 바꿔보세요.
    return time / 100; 
}