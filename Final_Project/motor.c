#include <device_driver.h>
#include <stdio.h>

// =========================================================
// [1] 모터 및 통신 핀 초기화
// =========================================================
void Motor_Init(void)
{
    // --- 1. 기존 DC 모터 제어 핀 설정 (PA0, PA1 -> TIM2 PWM) ---
    Macro_Write_Block(GPIOA->MODER, 0x3, 2, 0); 
    Macro_Write_Block(GPIOA->MODER, 0x3, 2, 2); 
    Macro_Clear_Bit(GPIOA->OTYPER, 0); 
    Macro_Clear_Bit(GPIOA->OTYPER, 1); 
    Macro_Write_Block(GPIOA->OSPEEDR, 0x3, 3, 0); 
    Macro_Write_Block(GPIOA->OSPEEDR, 0x3, 3, 2); 
    Macro_Write_Block(GPIOA->AFR[0], 0xF, 1, 0); 
    Macro_Write_Block(GPIOA->AFR[0], 0xF, 1, 4); 

    // --- 2. [추가] 서보 모터 설정 (PA6 -> TIM3_CH1) ---
    Macro_Set_Bit(RCC->APB1ENR, 1);    // TIM3 클럭 활성화
    
    // PA6 핀을 Alternate Function(10) 모드로 설정 (12번째 비트부터 2칸)
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 12); 
    
    // PA6를 AF2(TIM3)로 연결 (AFR[0]의 24번째 비트부터 4칸)
    Macro_Write_Block(GPIOA->AFR[0], 0xF, 0x2, 24);

    // TIM3 설정 (서보 모터용 50Hz PWM 생성)
    // 84MHz / 84 = 1,000,000Hz (1us 단위)
    TIM3->PSC = 84 - 1; 
    // 1,000,000 / 20,000 = 50Hz (20ms 주기)
    TIM3->ARR = 20000 - 1; 

    // PWM 모드 1 설정 (CH1)
    TIM3->CCMR1 |= (6 << 4); 
    // 출력 활성화
    TIM3->CCER |= (1 << 0);
    // 타이머 시작
    TIM3->CR1 |= (1 << 0);

    // --- 3. 기존 스텝 모터 설정 (PC0~PC3) ---
    RCC->AHB1ENR |= (1 << 2); 
    GPIOC->MODER &= ~(0xFF << 0);
    GPIOC->MODER |=  (0x55 << 0);
}

// 서보 모터 작동 함수
void Servo_Open_Close(void)
{
    // 1. 90도로 이동 (보통 1.5ms 펄스 = CCR값 1500)
    // 서보 모델마다 차이가 있을 수 있으니 1500~2000 사이에서 조절하세요.
    TIM3->CCR1 = 800; 
    printf("[SERVO] Lid Opening (90 deg)...\r\n");

    // 2. 2초간 상태 유지
    TIM2_Delay(2000);

    // 3. 다시 원래 자리(0도)로 복귀 (보통 0.5ms 펄스 = CCR값 500)
    TIM3->CCR1 = 2000; 
    printf("[SERVO] Lid Closing (0 deg)...\r\n");
    
    // 복귀할 시간 여유를 조금 줍니다.
    TIM2_Delay(500);
}

// =========================================================
// [2] 스텝 모터 구동 로직
// =========================================================

// 스텝 모터 1스텝 전진 (레지스터 직접 제어)
void Stepper_Step(int step_num) {
    unsigned int temp = GPIOC->ODR;
    
    // PC0~PC3 비트만 0으로 깔끔하게 지우기
    temp &= ~(0xF << 0); 
    
    // 스텝에 맞게 핀(전자석) 켜기
    switch(step_num % 4) {
        case 0: temp |= 0x09; break; // 1001
        case 1: temp |= 0x03; break; // 0011
        case 2: temp |= 0x06; break; // 0110
        case 3: temp |= 0x0C; break; // 1100
    }
    
    // 레지스터에 적용
    GPIOC->ODR = temp;
}

// 12도 회전 함수 (68 스텝 구동)
void Rotate_Next_Slot(void) {
    static int current_step = 0; 

    for(int i = 0; i < 2282; i++) {
        Stepper_Step(current_step);
        current_step++;

        // 한 스텝마다 2ms 대기
        TIM2_Delay(5); 
    }
    
    // 회전 완료 후 대기 상태일 때 모터 발열 방지 (전류 차단)
    GPIOC->ODR &= ~(0xF << 0); 
}

void Motor_Stop(void)
{
    TIM2->CCR1 = 0;
    TIM2->CCR2 = 0;
}

void Motor_Clockwise(unsigned int duty)
{
    Motor_Stop();
    for(volatile int i = 0; i < 0x5000; i++);

    TIM2->CCR1 = duty;
    TIM2->CCR2 = 0;
}

void Motor_CounterClockwise(unsigned int duty)
{
    Motor_Stop();
    for(volatile int i = 0; i < 0x5000; i++);

    TIM2->CCR1 = 0;
    TIM2->CCR2 = duty;
}

