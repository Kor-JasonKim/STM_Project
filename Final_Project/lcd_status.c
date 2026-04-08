#include "device_driver.h"

#define LCD2_ADDR 0x4C
#define STATE_IDLE      0
#define STATE_FORWARD   1
#define STATE_WAIT      2
#define STATE_BACKWARD  3
#define STATE_FINISHED  4

void LCD2_Show_State(int state, int dist)
{
    LCD_Set_Cursor_To(LCD2_ADDR, 0, 0);

    switch(state)
    {
        case STATE_IDLE:
            LCD_String_To(LCD2_ADDR, "State: IDLE    ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Waiting Alarm  ");
            break;
        case STATE_FORWARD:
            LCD_String_To(LCD2_ADDR, "State: FORWARD ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Pill Moving... ");
            break;
        case STATE_WAIT:
            LCD_String_To(LCD2_ADDR, "State: WAIT BTN");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Press Button   ");
            break;
        case STATE_BACKWARD:
            LCD_String_To(LCD2_ADDR, "State: BACKWARD");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Returning...   ");
            break;
        case STATE_FINISHED:
            LCD_String_To(LCD2_ADDR, "State: DONE    ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Sequence End   ");
            break;
        default:
            LCD_String_To(LCD2_ADDR, "State: UNKNOWN ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Check System   ");
            break;
    }
}