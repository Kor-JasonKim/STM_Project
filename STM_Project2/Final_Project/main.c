#include "device_driver.h"
#include <stdio.h>

#define REST 0

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

// [수정] Sys_Init에서 UART1, UART2, 패시브 부저용 TIM3 출력까지 켭니다.
static void Sys_Init(void)
{
    SCB->CPACR |= (0x3 << 10*2) | (0x3 << 11*2);
    Clock_Init();
    Uart2_Init(115200); // PC 테라텀용
    Uart1_Init(9600);   // 블루투스 모듈용
    setvbuf(stdout, NULL, _IONBF, 0);

    LED_Init();
    Motor_Init();
    LCD_Init();
    Ultrasonic_Init();
    Key_Poll_Init();

    // 패시브 부저용: PB0 = TIM3_CH3
    TIM3_Out_Init();
    TIM3_Out_Stop();
}

// 패시브 부저 ON/OFF 래퍼 함수
static void Passive_Buzzer_On(void)
{
    printf("[DEBUG] Calling TIM3_Out_Freq_Generation(2000)\r\n");
    TIM3_Out_Freq_Generation(2000);   // 2kHz
    buzzer_state = 1;
}

static void Passive_Buzzer_Off(void)
{
    printf("[DEBUG] Calling TIM3_Out_Stop()\r\n");
    TIM3_Out_Stop();
    buzzer_state = 0;
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
    printf("Passive Buzzer : PB0 (TIM3_CH3)\r\n");

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
            Rotate_Next_Slot();
            Servo_Open_Close();
            pill_alarm_flag = 0;
            printf("\r\n");

            if (current_state == STATE_IDLE || current_state == STATE_FINISHED) {
                printf("[1/4] Pill dropped. Conveyor Auto-Start...\r\n");
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
                    Rotate_Next_Slot();
                    Servo_Open_Close();

                    printf("[1/4] Manual Start. Moving Forward...\n");
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
                    Passive_Buzzer_On();

                    printf("[2/4] 1cm detected. Motor stop, passive buzzer ON.\r\n");
                    printf("Press USER button to stop buzzer and reverse.\r\n");

                    current_state = STATE_WAIT;
                }
                break;

            case STATE_WAIT:
                if (Key_Get_Pressed()) {
                    printf("[DEBUG] USER button pressed\r\n");
                    Passive_Buzzer_Off();

                    printf("[3/4] Button pressed. Buzzer OFF. Reversing...\r\n");

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
                    Passive_Buzzer_Off();

                    printf("\r\n[4/4] Reached 15cm. Sequence Finished.\r\n");
                    current_state = STATE_FINISHED;
                }
                break;
        }

        Delay_ms(100);
    }
}