#include "device_driver.h"
#include <stdio.h>

#define REST 0

volatile int system_mode = 0;
volatile int pill_alarm_flag = 0;

// [수정] Sys_Init에서 UART1과 UART2를 모두 켭니다.
static void Sys_Init(void) 
{
    SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
    Clock_Init();
    Uart2_Init(115200); // PC 테라텀용 (빠른 속도)
    Uart1_Init(9600);   // 블루투스 모듈용 (기본 속도 고정)
    setvbuf(stdout, NULL, _IONBF, 0);
    LED_Init();
    Motor_Init();    
    LCD_Init();
}

void Main(void) {
    Sys_Init(); 
    RTC_Init_And_Alarm_Set(0, 0, 0); 

    static int last_sec = -1;

    while(1) {
        // 1. 스마트폰(UART1)에서 날아오는 명령 처리
        Process_UART_Input();

        // 2. 실시간 시계 출력 로직
        unsigned int tr = RTC->TR; 
        
        int s = BCD_TO_DEC(tr & 0x7F);          
        int m = BCD_TO_DEC((tr >> 8) & 0x7F);   
        int h = BCD_TO_DEC((tr >> 16) & 0x3F);  

        // 시간이 1초 흘렀을 때만 화면 갱신
        if (s != last_sec) {
            // 1. 테라텀(PC) 모니터링 로그
            unsigned int alr = RTC->ALRMAR;
            int as = BCD_TO_DEC(alr & 0x7F);
            int am = BCD_TO_DEC((alr >> 8) & 0x7F);
            int ah = BCD_TO_DEC((alr >> 16) & 0x3F);
            
            printf("\r[PC] Time %02d:%02d:%02d | Alarm %02d:%02d:%02d  ", h, m, s, ah, am, as);

            // =========================================================
            // 2. LCD 화면에 예쁘게 출력하기 (스프린트 함수 이용)
            // =========================================================
            char lcd_buffer[20]; 

            // 윗줄: 현재 시간 (예: Clock : 15:30:00)
            sprintf(lcd_buffer, "Clock : %02d:%02d:%02d", h, m, s);
            LCD_Set_Cursor(0, 0); // (0번째 줄, 0번째 칸)
            LCD_String(lcd_buffer);

            // 아랫줄: 알람 시간 (예: Alarm : 08:00:00)
            sprintf(lcd_buffer, "Alarm : %02d:%02d:%02d", ah, am, as);
            LCD_Set_Cursor(1, 0); // (1번째 줄, 0번째 칸)
            LCD_String(lcd_buffer);
            // =========================================================

            last_sec = s; 
        }

        // 3. 알람 발생 시 동작
        if(pill_alarm_flag == 1) {
            printf("\r\n\n[ACTION] It's pill time! Rotating...\r\n");
            Rotate_Next_Slot(); 
            Servo_Open_Close();
            pill_alarm_flag = 0;
            printf("\r\n"); 
        }
    }
}