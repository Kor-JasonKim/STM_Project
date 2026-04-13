#include "device_driver.h"

// I2C LCD의 기본 주소
// LCD를 2개 쓰기 위해 주소를 2개로 분리
#define LCD1_ADDR 0x4E // 메인 LCD (0x27 << 1)
#define LCD2_ADDR 0x4C // 두번쨰 LCD   (0x26 << 1)

// I2C 통신 설정 (PB8: SCL, PB9: SDA)
void I2C1_Init(void) {
    // 1. 클럭 활성화 (GPIOB, I2C1)
    Macro_Set_Bit(RCC->AHB1ENR, 1);  
    Macro_Set_Bit(RCC->APB1ENR, 21); 

    // 2. PB8, PB9 핀 설정 (Alternate Function, Open-Drain)
    Macro_Write_Block(GPIOB->MODER, 0xF, 0xA, 16);   // AF 모드
    Macro_Write_Block(GPIOB->OTYPER, 0x3, 0x3, 8);   // Open Drain (I2C 필수)
    Macro_Write_Block(GPIOB->PUPDR, 0xF, 0x5, 16);   // Pull-Up
    Macro_Write_Block(GPIOB->AFR[1], 0xFF, 0x44, 0); // AF4 (I2C1)

    // 3. I2C1 소프트웨어 리셋
    Macro_Set_Bit(I2C1->CR1, 15);
    Macro_Clear_Bit(I2C1->CR1, 15);

    // 4. 통신 속도 설정 (APB1 클럭 42MHz)
    I2C1->CR2 = 42;           
    I2C1->CCR = 210;          // 100kHz Standard Mode
    I2C1->TRISE = 43;
    
    // 5. I2C 활성화
    Macro_Set_Bit(I2C1->CR1, 0); 
}


// 주소를 직접 지정해서 보내는 함수
void I2C1_Write_Byte_To(unsigned char addr, char data) {
    while(I2C1->SR2 & (1<<1));     // 버스가 비어있을 때까지 대기
    I2C1->CR1 |= (1<<8);           // START 조건 생성
    while(!(I2C1->SR1 & (1<<0)));  // SB 대기
    
    I2C1->DR = addr;               // LCD_ADDR 대신 인자로 받은 addr 전송
    while(!(I2C1->SR1 & (1<<1)));  // ADDR 대기
    (void)I2C1->SR1; 
    (void)I2C1->SR2;               // 플래그 클리어
    
    while(!(I2C1->SR1 & (1<<7)));  // TXE 대기
    I2C1->DR = data;               // 데이터 전송
    while(!(I2C1->SR1 & (1<<2)));  // BTF 대기
    
    I2C1->CR1 |= (1<<9);           // STOP 조건 생성
}

// I2C 데이터 전송 함수
void I2C1_Write_Byte(char data) {
    I2C1_Write_Byte_To(LCD1_ADDR, data);
}


// LCD 제어 로직 (PCF8574)
// 주소 지정형 LCD 전송 함수
void LCD_Send_To(unsigned char addr, char data, int mode) {
    char high_nib = data & 0xF0;
    char low_nib  = (data << 4) & 0xF0;
    char bl = 0x08; // 백라이트 ON (OFF하려면 0x00)

    // 상위 4비트 전송 (EN=1 -> EN=0)
    I2C1_Write_Byte_To(addr, high_nib | mode | bl | 0x04);
    for(volatile int i=0; i<5000; i++); // 미세 딜레이
    I2C1_Write_Byte_To(addr, high_nib | mode | bl | 0x00);

    // 하위 4비트 전송 (EN=1 -> EN=0)
    I2C1_Write_Byte_To(addr, low_nib | mode | bl | 0x04);
    for(volatile int i=0; i<5000; i++);
    I2C1_Write_Byte_To(addr, low_nib | mode | bl | 0x00);
}

// 주소 지정형 명령 전송 함수
void LCD_Cmd_To(unsigned char addr, char cmd) { LCD_Send_To(addr, cmd, 0x00); }    // 명령어 전송 (RS=0)

// 주소 지정형 데이터 전송 함수
void LCD_Data_To(unsigned char addr, char data) { LCD_Send_To(addr, data, 0x01); } // 글자 전송 (RS=1)

// LCD1에 전송
void LCD_Send(char data, int mode) {
    LCD_Send_To(LCD1_ADDR, data, mode);
}

void LCD_Cmd(char cmd) { LCD_Send_To(LCD1_ADDR, cmd, 0x00); }    //    기본 LCD1로 명령 전송
void LCD_Data(char data) { LCD_Send_To(LCD1_ADDR, data, 0x01); } //    기본 LCD1로 데이터 전송

// 주소 지정형 LCD 초기화
void LCD_Init_To(unsigned char addr) {
    TIM2_Delay(50);     // 전원 켜지고 대기
    
    LCD_Cmd_To(addr, 0x02);      // 4-bit 모드 초기화
    LCD_Cmd_To(addr, 0x28);      // 2줄 출력, 5x8 폰트
    LCD_Cmd_To(addr, 0x0C);      // 화면 ON, 커서 OFF
    LCD_Cmd_To(addr, 0x06);      // 글자 쓸 때 커서 우측 이동
    LCD_Cmd_To(addr, 0x01);      // 화면 깨끗하게 지우기
    TIM2_Delay(5);
}

void LCD_Init(void) {
    I2C1_Init();
    LCD_Init_To(LCD1_ADDR); // LCD만 초기화
}

// 원하는 위치로 커서 이동 (x: 가로칸 0~15, y: 세로줄 0~1)
void LCD_Set_Cursor_To(unsigned char addr, int y, int x) {
    if (y == 0) LCD_Cmd_To(addr, 0x80 + x); // 첫 번째 줄
    else        LCD_Cmd_To(addr, 0xC0 + x); // 두 번째 줄
}

void LCD_Set_Cursor(int y, int x) {
    LCD_Set_Cursor_To(LCD1_ADDR, y, x);
}

// 주소 지정형 문자열 출력
void LCD_String_To(unsigned char addr, char *str) {
    while(*str) LCD_Data_To(addr, *str++);
}

void LCD_String(char *str) {
    LCD_String_To(LCD1_ADDR, str);
}