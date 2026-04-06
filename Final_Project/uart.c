#include "device_driver.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void Uart2_Init(int baud)
{
  double div;
  unsigned int mant;
  unsigned int frac;

  Macro_Set_Bit(RCC->AHB1ENR, 0);                   // PA2,3
  Macro_Set_Bit(RCC->APB1ENR, 17);                   // USART2 ON
  Macro_Write_Block(GPIOA->MODER, 0xf, 0xa, 4);     // PA2,3 => ALT
  Macro_Write_Block(GPIOA->AFR[0], 0xff, 0x77, 8);  // PA2,3 => AF07
  Macro_Write_Block(GPIOA->PUPDR, 0xf, 0x5, 4);     // PA2,3 => Pull-Up  

  volatile unsigned int t = GPIOA->LCKR & 0x7FFF;
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<2);                // Lock PA2, 3 Configuration
  GPIOA->LCKR = (0x0<<16)|t|(0x3<<2);
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<2);
  t = GPIOA->LCKR;

  div = PCLK1/(16. * baud);
  mant = (int)div;
  frac = (int)((div - mant) * 16. + 0.5);
  mant += frac >> 4;
  frac &= 0xf;

  USART2->BRR = (mant<<4)|(frac<<0);
  USART2->CR1 = (1<<13)|(0<<12)|(0<<10)|(1<<3)|(1<<2);
  USART2->CR2 = 0<<12;
  USART2->CR3 = 0;
}

void Uart2_Send_Byte(char data)
{
  if(data == '\n')
  {
    while(!Macro_Check_Bit_Set(USART2->SR, 7));
    USART2->DR = 0x0d;
  }

  while(!Macro_Check_Bit_Set(USART2->SR, 7));
  USART2->DR = data;
}

void Uart1_Init(int baud)
{
  double div;
  unsigned int mant;
  unsigned int frac;

  Macro_Set_Bit(RCC->AHB1ENR, 0);                   // PA9,10
  Macro_Set_Bit(RCC->APB2ENR, 4);                   // USART1 ON
  Macro_Write_Block(GPIOA->MODER, 0xf, 0xa, 18);    // PA9,10 => ALT
  Macro_Write_Block(GPIOA->AFR[1], 0xff, 0x77, 4);  // PA9,10 => AF07
  Macro_Write_Block(GPIOA->PUPDR, 0xf, 0x5, 18);    // PA9,10 => Pull-Up
  
  volatile unsigned int t = GPIOA->LCKR & 0x7FFF;
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<9);               // Lock PA9, 10 Configuration
  GPIOA->LCKR = (0x0<<16)|t|(0x3<<9);
  GPIOA->LCKR = (0x1<<16)|t|(0x3<<9);
  t = GPIOA->LCKR;

  div = PCLK2 / (16. * baud);
  mant = (int)div;
  frac = (int)((div - mant) * 16 + 0.5);
  mant += frac >> 4;
  frac &= 0xf;
  USART1->BRR = (mant<<4)|(frac<<0);

  USART1->CR1 = (1<<13)|(0<<12)|(0<<10)|(1<<3)|(1<<2);
  USART1->CR2 = 0 << 12;
  USART1->CR3 = 0;
}

void Uart1_Send_Byte(char data)
{
  if(data == '\n')
  {
    while(!Macro_Check_Bit_Set(USART1->SR, 7));
    USART1->DR = 0x0d;
  }

  while(!Macro_Check_Bit_Set(USART1->SR, 7));
  USART1->DR = data;
}

void Uart1_Send_String(char *pt)
{
  while(*pt != 0)
  {
    Uart1_Send_Byte(*pt++);
  }
}

void Uart1_Printf(char *fmt,...)
{
	va_list ap;
	char string[256];

	va_start(ap,fmt);
	vsprintf(string,fmt,ap);
	Uart1_Send_String(string);
	va_end(ap);
}

char Uart1_Get_Pressed(void)
{
	if(Macro_Check_Bit_Set(USART1->SR, 5))
	{
		return (char)USART1->DR;
	}

	else
	{
		return (char)0;
	}
}

char Uart2_Get_Char(void)
{
	while(!Macro_Check_Bit_Set(USART2->SR, 5));
	return (char)USART2->DR;
}

void Uart2_Send_String(char *pt)
{
  while(*pt != 0)
  {
    Uart2_Send_Byte(*pt++);
  }
}

void Uart2_Printf(char *fmt,...)
{
  va_list ap;
  char string[256];

  va_start(ap,fmt);
  vsprintf(string,fmt,ap);
  Uart2_Send_String(string);
  va_end(ap);
}

char rx_buffer[20];
int rx_index = 0;

void Process_UART_Input(void) 
{
    // [수정] USART2 -> USART1 로 모두 변경
    if (USART1->SR & (1 << 5)) { 
        char ch = (char)(USART1->DR & 0xFF); 
        
        if (ch == '\r' || ch == '\n') {
            rx_buffer[rx_index] = '\0'; 
            
            if (rx_index > 0) {
                int h, m, s;
                
                // 1. 시간 동기화 (T)
                if (rx_buffer[0] == 'T' || rx_buffer[0] == 't') {
                    if (sscanf(rx_buffer + 1, " %d:%d:%d", &h, &m, &s) == 3) {
                        Set_Current_Time(h, m, s);
                        // [수정] PC 터미널과 앱에 각각 따로 알려줍니다.
                        printf("\r\n[SYNC] Current Time Synced -> %02d:%02d:%02d\r\n", h, m, s);
                        Uart1_Printf("T:%02d:%02d:%02d\n", h, m, s); // 앱으로 전송
                    }
                }
                // 2. 알람 설정 (A)
                else if (rx_buffer[0] == 'A' || rx_buffer[0] == 'a') {
                    if (sscanf(rx_buffer + 1, " %d:%d:%d", &h, &m, &s) == 3) {
                        RTC->WPR = 0xCA; RTC->WPR = 0x53;
                        RTC->CR &= ~(1 << 8); 
                        while(!(RTC->ISR & (1 << 0))); 
                        
                        RTC->ALRMAR = (1 << 31) | (TO_BCD(h) << 16) | (TO_BCD(m) << 8) | TO_BCD(s);
                        
                        RTC->CR |= (1 << 8); 
                        RTC->WPR = 0xFF;     
                        
                        // [수정] 설정된 알람 시간을 앱으로 바로 피드백 전송
                        printf("\r\n[SET] Alarm Updated -> %02d:%02d:%02d\r\n", h, m, s);
                        Uart1_Printf("A:%02d:%02d:%02d\n", h, m, s); // 앱으로 전송
                    }
                } else {
                    printf("\r\n[ERROR] Invalid Command\r\n");
                }
            }
            rx_index = 0; 
        } else if (rx_index < 19) {
            rx_buffer[rx_index++] = ch;
        }
    }
}
