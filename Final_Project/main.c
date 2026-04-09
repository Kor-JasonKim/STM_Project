#include "device_driver.h"
#include <stdio.h>

#define REST 0

// [추가] LCD 주소 정의
#define LCD1_ADDR 0x4E
#define LCD2_ADDR 0x4C

#define STATE_IDLE      0
#define STATE_FORWARD   1
#define STATE_WAIT      2
#define STATE_BACKWARD  3
#define STATE_FINISHED  4

volatile int system_mode = 0;
volatile int pill_alarm_flag = 0;
int current_state = STATE_IDLE;

// 부저 상태 디버그용
volatile int buzzer_state = 0;

// [디버그] 테라텀으로 제어하기 
volatile int Uart_Data_In = 0; // flag, 
volatile unsigned char Uart_Data = 0;
//

static void Sys_Init(void)
{
    SCB->CPACR |= (0x3 << 10*2) | (0x3 << 11*2);
    Clock_Init();
    Uart2_Init(115200); // PC 테라텀용
    Uart1_Init(9600);   // 블루투스 모듈용
    setvbuf(stdout, NULL, _IONBF, 0);

    LED_Init();
    Status_LED_Init(); // [추가] 외부 상태 표시 LED 6개 초기화
    Motor_Init();
    LCD_Init();               // [유지] 기존 LCD(0x4E) 초기화
    LCD_Init_To(LCD2_ADDR);   // [추가] 두 번째 LCD(0x4C) 초기화
    Ultrasonic_Init();
    Key_Poll_Init();

    // 패시브 부저용: TIM4
    Buzzer_Init();

    Uart2_RX_Interrupt_Enable();
}

 
void Main(void)
{
    Sys_Init();
    RTC_Init_And_Alarm_Set(0, 0, 0);

    static int last_sec = -1;
    char key;
    int dist;

    printf("=== Integrated Pill Dispenser System Ready ===\r\n");
    printf("Waiting for Alarm or Press 'f' for manual test.\r\n");
    printf("Passive Buzzer : PB6 (TIM4_CH1)\r\n");
    
    // 시작할 때 모든 외부 LED 끄기
    Status_LED_All_Off();

    while(1)
    {
        // 1. 스마트폰(UART1)에서 날아오는 명령 처리
        Process_UART_Input();

        // 2. 실시간 시계 출력 로직
        unsigned int tr = RTC->TR;

        int s = BCD_TO_DEC(tr & 0x7F);
        int m = BCD_TO_DEC((tr >> 8) & 0x7F);
        int h = BCD_TO_DEC((tr >> 16) & 0x3F);

        // 시간이 1초 흘렀을 때만 화면 갱신
        if (s != last_sec) {
            unsigned int alr = RTC->ALRMAR;
            int as = BCD_TO_DEC(alr & 0x7F);
            int am = BCD_TO_DEC((alr >> 8) & 0x7F);
            int ah = BCD_TO_DEC((alr >> 16) & 0x3F);

            printf("\r[PC] Time %02d:%02d:%02d | Alarm %02d:%02d:%02d | Buzzer %d  ",
                   h, m, s, ah, am, as, buzzer_state);

            char lcd_buffer[20];

            sprintf(lcd_buffer, "Clock : %02d:%02d:%02d", h, m, s);
            LCD_Set_Cursor(0, 0);
            LCD_String(lcd_buffer);

            sprintf(lcd_buffer, "Alarm : %02d:%02d:%02d", ah, am, as);
            LCD_Set_Cursor(1, 0);
            LCD_String(lcd_buffer);

            last_sec = s;
        }

        // 3. 알람 발생 시 동작
        if (pill_alarm_flag == 1) {
            printf("\r\n\r\n[ACTION] It's pill time! Rotating...\r\n");
            // [추가] 약이 배출될 때: 빨간색 LED ON
            Status_LED_Red();

            Rotate_Next_Slot();
            Servo_Open_Close();
            for(volatile int i=0;i<0x100000;i++);
            Rotate_Next_Slot();
            for(volatile int i=0;i<0x100000;i++);
            Supply_Pill();
            pill_alarm_flag = 0;
            printf("\r\n");

            if (current_state == STATE_IDLE || current_state == STATE_FINISHED) {
                printf("[1/4] Pill dropped. Conveyor Auto-Start...\r\n");

                // [추가] 컨베이어 이동 시작: 노란색 LED ON!
                Status_LED_Yellow();                
                Motor_Set_Percent(50);
                Move_CW();
                current_state = STATE_FORWARD;
            }
        }

        // 4. 컨베이어 벨트 상태 머신 및 거리 측정
        dist = Ultrasonic_Get_Distance();

        switch (current_state)
        {
            case STATE_IDLE:
            case STATE_FINISHED:
                key = Uart2_Get_Pressed();
                if (key == 'f' || key == 'F') {
                    printf("\n[ACTION] Manual pill drop...\n");
                    // [추가] 수동 약 배출: 빨간색 LED ON!
                    Status_LED_Red();
                    Rotate_Next_Slot();
                    Servo_Open_Close();

                    printf("[1/4] Manual Start. Moving Forward...\n");
                    // [추가] 컨베이어 전진 시작: 노란색 LED ON!
                    Status_LED_Yellow();
                    Motor_Set_Percent(50);
                    Move_CW();
                    current_state = STATE_FORWARD;
                }
                break;

            case STATE_FORWARD:
                // 필요하면 <=1 을 <=3 으로 완화해서 테스트하세요
                if (dist > 0 && dist <= 1) {
                    Stop();

                    printf("\r\n[DEBUG] dist = %d -> Stop()\r\n", dist);
                    Buzzer_On();

                    // [추가] 도착해서 멈추면 일단 LED off
                    Status_LED_All_Off();

                    printf("[2/4] 1cm detected. Motor stop, passive buzzer ON.\r\n");
                    printf("Press external switch(PB5) button to stop buzzer and reverse.\r\n");

                    current_state = STATE_WAIT;
                }
                break;

            case STATE_WAIT:
                if (Key_Get_Pressed()) {
                    printf("[DEBUG] External switch pressed\r\n");
                    Buzzer_Off();

                    printf("[3/4] Button pressed. Buzzer OFF. Reversing...\r\n");
                    // [추가] 역회전 복귀 시작: 초록색 LED ON!
                    Status_LED_Green();

                    Motor_Set_Percent(50);
                    Move_CCW();

                    Key_Wait_Key_Released();
                    TIM2_Delay(50);
                    current_state = STATE_BACKWARD;
                }
                break;

            case STATE_BACKWARD:
                if (dist >= 15) {
                    Stop();
                    Buzzer_Off();

                    // [추가] 복귀 완료 후 LED 모두 끄기
                    Status_LED_All_Off();


                    printf("\r\n[4/4] Reached 15cm. Sequence Finished.\r\n");
                    current_state = STATE_FINISHED;
                }
                break;
        }

        LCD2_Show_State(current_state, dist); // [수정] 상태값을 인자로 넘겨서 LCD2 표시
        Delay_ms(100);


        char debug_command = Uart_Data;

        if (Uart_Data_In) 
        {
            int h = 0;
            int m = 0;
            int s = 0;
            
            // 1. 시간 동기화 (T)
            if (debug_command == 'T' || debug_command == 't') 
            {


                Set_Current_Time(h, m, s);
                printf("\r\n[SYNC] Current Time Synced -> %02d:%02d:%02d\r\n", h, m, s);
                Uart1_Printf("T:%02d:%02d:%02d\n", h, m, s); 
                
            }

            // 2. 알람 설정 (A)
            else if (debug_command == 'A' || debug_command == 'a') 
            {
                int h = 0;
                int m = 0;
                int s = 5;
                

            RTC->WPR = 0xCA; RTC->WPR = 0x53;
            RTC->CR &= ~(1 << 8); 
            while(!(RTC->ISR & (1 << 0))); 
            
            RTC->ALRMAR = (1 << 31) | (TO_BCD(h) << 16) | (TO_BCD(m) << 8) | TO_BCD(s);
            
            RTC->CR |= (1 << 8); 
            RTC->WPR = 0xFF;     
            
            printf("\r\n[SET] Alarm Updated -> %02d:%02d:%02d\r\n", h, m, s);
            Uart1_Printf("A:%02d:%02d:%02d\n", h, m, s); 
                
            }

            //스테퍼모터 설정
            else if( debug_command == 's')
            {
                printf("\nstepper start");
                int temp = 0;
                for (int k = 0; k < 1144; k++)
                {
                    Stepper2_Step(temp);
                    temp++;
                    TIM2_Delay(5); 
                    printf("%d\n", temp);
                }
                
                
            }
            

            Uart_Data_In = 0;
        }



    }
}
