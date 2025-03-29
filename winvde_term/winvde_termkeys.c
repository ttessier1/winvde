#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <conio.h>

#include "winvde_termkeys.h"
#include "winvde_cmdparam.h"
#include "winvde_cmdhist.h"
#include "winvde_confuncs.h"
#include "winvde_term.h"


uint8_t insertMode = 0;
uint8_t bufferReady = 0;

uint16_t std_input_pos = 0;
uint16_t std_input_length = 0;
char std_input_buffer[STD_INPUT_BUFF_SIZE];
char std_input_cpy_buffer[STD_INPUT_BUFF_SIZE];

struct key_handlers GlobalKeyHandler = {
    .KyBdF1 = DefKbHit,
    .KyBdF2 = DefKbHit,
    .KyBdF3 = DefKbHit,
    .KyBdF4 = DefKbHit,
    .KyBdF5 = DefKbHit,
    .KyBdF6 = DefKbHit,
    .KyBdF7 = DefKbHit,
    .KyBdF8 = DefKbHit,
    .KyBdF9 = DefKbHit,
    .KyBdF10 = DefKbHit,
    .KyBdF12 = DefKbHit,
    .NumPadDot = DefKbHit,
    .NumPadZero = DefKbHit,
    .NumPadUp = DefKbHit,
    .NumPadDown = DefKbHit,
    .NumPadLeft = DefKbHit,
    .NumPadRight = DefKbHit,
    .NumPadPgUp = DefKbHit,
    .NumPadPgDwn = DefKbHit,
    .NumPadHome = DefKbHit,
    .NumPadEnd = DefKbHit,
    .KyBdUp = DefKbHit,
    .KyBdDown = DefKbHit,
    .KyBdLeft = DefKbHit,
    .KyBdRight = DefKbHit,
    .KyBdDel = DefKbHit,
    .KyBdInsert = DefKbHit,
    .KyBdEnd = DefKbHit,
    .KyBdHome = DefKbHit,
    .KyBdPgUp = DefKbHit,
    .KyBdPgDwn = DefKbHit,
    .KyBdEsc = DefKbHit,
};

void DefKbHit()
{
    // Do Nothing
}

int DoKbHit()
{
    struct command_parameter* command = NULL;
    struct command_history* cmd_history = NULL;
    struct command_history* ptr = NULL;
    int8_t input_char;
    if (_kbhit())
    {
        input_char = _getch();// dont output
        if (input_char == '\b') // backspace
        {
            if (std_input_pos > 0)
            {
                _putch('\b');
                _putch(' ');
                _putch(input_char);
                std_input_pos--;
                std_input_length--;
                return 0;
            }
        }
        else if (input_char == 0)
        {
            input_char = _getch();
            switch (input_char)
            {
            case 'S': // numpad . with numlock off
                if (GlobalKeyHandler.NumPadDot)
                {
                    (*GlobalKeyHandler.NumPadDot)();
                }
                return 0;
            case 'R': // numpad 0
                if (GlobalKeyHandler.NumPadZero)
                {
                    (*GlobalKeyHandler.NumPadZero)();
                }
                return 0;
            case 'H': //numpad up
                if (GlobalKeyHandler.NumPadUp)
                {
                    GlobalKeyHandler.NumPadUp();
                }
                return 0;
            case 'P': //numpad down
                if (GlobalKeyHandler.NumPadDown)
                {
                    (*GlobalKeyHandler.NumPadDown)();
                }
                return 0;
            case 'K': //mumpad left
                if (GlobalKeyHandler.NumPadLeft)
                {
                    (*GlobalKeyHandler.NumPadLeft)();
                }
                return 0;
            case 'M': // numpad right
                if (GlobalKeyHandler.NumPadRight)
                {
                    (*GlobalKeyHandler.NumPadRight)();
                }
                return 0;
            case 'G':// numpad home
                if (GlobalKeyHandler.NumPadHome)
                {
                    (*GlobalKeyHandler.NumPadHome)();
                }
                return 0;
            case 'O'://numpad end
                if (GlobalKeyHandler.NumPadEnd)
                {
                    (*GlobalKeyHandler.NumPadEnd)();
                }
                return 0;
            case 'I': // numpad pgup
                if (GlobalKeyHandler.NumPadPgUp)
                {
                    (*GlobalKeyHandler.NumPadPgUp)();
                }
                return 0;
            case 'Q': // numpad pgdn
                if (GlobalKeyHandler.NumPadPgDwn)
                {
                    (*GlobalKeyHandler.NumPadPgDwn)();
                }
                return 0;
            case 59: // 59 F1
                if (GlobalKeyHandler.KyBdF1)
                {
                    (*GlobalKeyHandler.KyBdF1)();
                }
                return 0;
            case 60: // 60 F2
                if (GlobalKeyHandler.KyBdF2)
                {
                    (*GlobalKeyHandler.KyBdF2)();
                }
                return 0;
            case 61: // 61 F3
                if (GlobalKeyHandler.KyBdF3)
                {
                    (*GlobalKeyHandler.KyBdF3)();
                }
                return 0;
            case 62: // 62 F4
                if (GlobalKeyHandler.KyBdF4)
                {
                    (*GlobalKeyHandler.KyBdF4)();
                }
                return 0;
            case 63: // 63 F5
                if (GlobalKeyHandler.KyBdF5)
                {
                    (*GlobalKeyHandler.KyBdF5)();
                }
                return 0;
            case 64: // 64 F6
                if (GlobalKeyHandler.KyBdF6)
                {
                    (*GlobalKeyHandler.KyBdF6)();
                }
                return 0;
            case 65: // 65 F7
                if (GlobalKeyHandler.KyBdF7)
                {
                    (*GlobalKeyHandler.KyBdF7)();
                }
                return 0;
            case 66: // 66 F8
                if (GlobalKeyHandler.KyBdF8)
                {
                    (*GlobalKeyHandler.KyBdF8)();
                }
                return 0;
            case 67: // 67 F9
                if (GlobalKeyHandler.KyBdF9)
                {
                    (*GlobalKeyHandler.KyBdF9)();
                }
                return 0;
            case 68: // 68 F10
                if (GlobalKeyHandler.KyBdF10)
                {
                    (*GlobalKeyHandler.KyBdF10)();
                }
                return 0;
                // case 5 key while numlock off does nothing
            default:
                //fprintf(stderr, "[Unknown -%d]", input_char);
                return -1;
            }
        }
        else if (input_char == -32)
        {
            input_char = _getch();
            switch (input_char)
            {
            case 'G'://home key
                if (GlobalKeyHandler.KyBdHome)
                {
                    (*GlobalKeyHandler.KyBdHome)();
                }
                return 0;
            case 'O': //end keu
                if (GlobalKeyHandler.KyBdEnd)
                {
                    (*GlobalKeyHandler.KyBdEnd)();
                }
                return 0;
            case 'R'://in key
                if (GlobalKeyHandler.KyBdInsert)
                {
                    (*GlobalKeyHandler.KyBdInsert)();
                }
                return 0;
            case 'S': //del key
                if (GlobalKeyHandler.KyBdInsert)
                {
                    (*GlobalKeyHandler.KyBdDel)();
                }
                return 0;
            case 'I':// pgup
                if (GlobalKeyHandler.KyBdPgUp)
                {
                    (*GlobalKeyHandler.KyBdPgUp)();
                }
                return 0;
            case 'Q'://pgdown
                if (GlobalKeyHandler.KyBdPgDwn)
                {
                    (*GlobalKeyHandler.KyBdPgDwn)();
                }
                return 0;
            case 'H'://up arrow
                if (GlobalKeyHandler.KyBdUp)
                {
                    (*GlobalKeyHandler.KyBdUp)();
                }
                return 0;
            case 'P': //down arrow
                if (GlobalKeyHandler.KyBdDown)
                {
                    (*GlobalKeyHandler.KyBdDown)();
                }
                return 0;
            case 'K': // left arrow
                if (GlobalKeyHandler.KyBdLeft)
                {
                    (*GlobalKeyHandler.KyBdLeft)();
                }
                return 0;
            case 'M'://right arrow
                if (GlobalKeyHandler.KyBdRight)
                {
                    (*GlobalKeyHandler.KyBdRight)();
                }
                return 0;
            case -122:// F12
                if (GlobalKeyHandler.KyBdF12)
                {
                    (*GlobalKeyHandler.KyBdF12)();
                }
                return 0;
            default:
                return -1;
            }
            //fprintf(stdout, "\033[32mSpecial Char:\033[30m %c %d %.*s\n", input_char, input_char, std_input_pos, std_input_buffer);
            std_input_buffer[std_input_pos] = input_char;
            std_input_pos++;
            if (std_input_pos >= STD_INPUT_BUFF_SIZE)
            {

                std_input_pos = 0;
                bufferReady = 1;

            }
        }
        else
        {
            if (input_char == 0x1b) // escape
            {
                if (GlobalKeyHandler.KyBdEsc)
                {
                    (*GlobalKeyHandler.KyBdEsc)();
                }
                return 0;
            }
            if (std_input_length == std_input_pos)
            {
                _putch(input_char);
            }
            else
            {
                if (insertMode == 0 && input_char != '\r' && !input_char != '\n')
                {
                    _putch(input_char);
                    //fprintf(stdout, "\0337");
                    //printf("%.*s", std_input_length - std_input_pos, &std_input_buffer[std_input_pos]);
                    //fprintf(stdout, "\0338");
                    memcpy(std_input_cpy_buffer, &std_input_buffer[std_input_pos], std_input_length - std_input_pos);
                    std_input_buffer[std_input_pos] = input_char;
                    memcpy(&std_input_buffer[std_input_pos + 1], std_input_cpy_buffer, min(STD_INPUT_BUFF_SIZE - std_input_pos, std_input_length - std_input_pos));
                    std_input_pos++;
                    std_input_length++;
                    return 0;
                }
            }

            if (input_char == '\n' || input_char == '\r')
            {
                bufferReady = 1;
                if (std_input_length > 0)
                {
                    if (ParseCommand(std_input_buffer, std_input_length, &command) == -1)
                    {
                        fprintf(stderr, "Failed to parse the command\n");
                        return -1;
                    }
                    //if (HandleCommand(command) == 0)
                    //{
                    //    fprintf(stdout, "\033[32m%s\033[0m", prompt);
                    //    SaveCursorPos();
                    //    std_input_buffer[std_input_length + 1] = '\0';
                    //    addCommandHistory(&cmd_history, std_input_buffer, std_input_length);
                    //    std_input_pos = 0;
                    //    std_input_length = 0;
                    //    bufferReady = 0;
                        return 0;
                    //}
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                std_input_buffer[std_input_pos] = input_char;
                std_input_pos++;
                std_input_length++;
                if (std_input_pos >= STD_INPUT_BUFF_SIZE)
                {

                    std_input_pos = 0;
                    std_input_length = 0;
                    bufferReady = 1;
                }
                return 0;
            }
        }
    }
    return 0 ;
}

int AttachKeyHandler(int32_t key, void(*func)())
{
    if (key<KEY_MIN || key>KEY_MAX || !func)
    {
        return -1;
    }
    switch (key)
    {
    case NUMPAD_DOT_KEY:
        GlobalKeyHandler.NumPadDot = func;
    break;
    case NUMPAD_0_KEY:
        GlobalKeyHandler.NumPadZero = func;
    break;
    case NUMPAD_UP_KEY:
        GlobalKeyHandler.NumPadUp = func;
    break;
    case NUMPAD_DOWN_KEY:
        GlobalKeyHandler.NumPadDown = func;
    break;
    case NUMPAD_LEFT_KEY :
        GlobalKeyHandler.NumPadLeft = func;
    break;
    case NUMPAD_RIGHT_KEY:
        GlobalKeyHandler.NumPadRight = func;
    break;
    case NUMPAD_HOME_KEY:
        GlobalKeyHandler.NumPadHome = func;
    break;
    case NUMPAD_END_KEY:
        GlobalKeyHandler.NumPadEnd = func;
    break;
    case NUMPAD_PGUP_KEY:
        GlobalKeyHandler.NumPadPgUp = func;
    break;
    case NUMPAD_PGDOWN_KEY:
        GlobalKeyHandler.NumPadPgDwn = func;
    break;
    case KEYBOARD_F1_KEY:
        GlobalKeyHandler.KyBdF1 = func;
    break;
    case KEYBOARD_F2_KEY:
        GlobalKeyHandler.KyBdF2 = func;
    break;
    case KEYBOARD_F3_KEY:
        GlobalKeyHandler.KyBdF3 = func;
    break;
    case KEYBOARD_F4_KEY:
        GlobalKeyHandler.KyBdF4 = func;
    break;
    case KEYBOARD_F5_KEY:
        GlobalKeyHandler.KyBdF5 = func;
        break;
    case KEYBOARD_F6_KEY:
        GlobalKeyHandler.KyBdF6 = func;
    break;
    case KEYBOARD_F7_KEY:
        GlobalKeyHandler.KyBdF7 = func;
        break;
    case KEYBOARD_F8_KEY:
        GlobalKeyHandler.KyBdF8 = func;
        break;
    case KEYBOARD_F9_KEY:
        GlobalKeyHandler.KyBdF9 = func;
        break;
    case KEYBOARD_F10_KEY:
        GlobalKeyHandler.KyBdF10 = func;
        break;
    case KEYBOARD_F12_KEY:
        GlobalKeyHandler.KyBdF12 = func;
        break;
    case KEYBOARD_UP:
	    GlobalKeyHandler.KyBdUp = func;
	    break;
    case KEYBOARD_DOWN:
	    GlobalKeyHandler.KyBdDown = func;
	    break;
    case KEYBOARD_LEFT:
	    GlobalKeyHandler.KyBdLeft = func;
	    break;
    case KEYBOARD_RIGHT:
	    GlobalKeyHandler.KyBdRight = func;
	    break;
    case KEYBOARD_DEL:
	    GlobalKeyHandler.KyBdDel = func;
	    break;
    case KEYBOARD_INSERT:
	    GlobalKeyHandler.KyBdInsert = func;
	    break;
    case KEYBOARD_END:
	    GlobalKeyHandler.KyBdEnd = func;
	    break;
    case KEYBOARD_HOME:
	    GlobalKeyHandler.KyBdHome = func;
	    break;
    case KEYBOARD_PGUP:
	    GlobalKeyHandler.KyBdPgUp = func;
	    break;
    case KEYBOARD_PGDWN:
	    GlobalKeyHandler.KyBdPgDwn = func;
	    break;
    case KEYBOARD_ESC:
	    GlobalKeyHandler.KyBdEsc = func;
	    break;

    default:
        return -1;
    }
    return 0;
}

int RemoveKeyHandler(uint8_t key, void(*func)())
{
    if (key<KEY_MIN || key>KEY_MAX || !func)
    {
        return -1;
    }
    switch (key)
    {
    case NUMPAD_DOT_KEY:
        GlobalKeyHandler.NumPadDot = DefKbHit;
        break;
    case NUMPAD_0_KEY:
        GlobalKeyHandler.NumPadZero = DefKbHit;
        break;
    case NUMPAD_UP_KEY:
        GlobalKeyHandler.NumPadUp = DefKbHit;
        break;
    case NUMPAD_DOWN_KEY:
        GlobalKeyHandler.NumPadDown = DefKbHit;
        break;
    case NUMPAD_LEFT_KEY:
        GlobalKeyHandler.NumPadLeft = DefKbHit;
        break;
    case NUMPAD_RIGHT_KEY:
        GlobalKeyHandler.NumPadRight = DefKbHit;
        break;
    case NUMPAD_HOME_KEY:
        GlobalKeyHandler.NumPadHome = DefKbHit;
        break;
    case NUMPAD_END_KEY:
        GlobalKeyHandler.NumPadEnd = DefKbHit;
        break;
    case NUMPAD_PGUP_KEY:
        GlobalKeyHandler.NumPadPgUp = DefKbHit;
        break;
    case NUMPAD_PGDOWN_KEY:
        GlobalKeyHandler.NumPadPgDwn = DefKbHit;
        break;
    case KEYBOARD_F1_KEY:
        GlobalKeyHandler.KyBdF1 = DefKbHit;
        break;
    case KEYBOARD_F2_KEY:
        GlobalKeyHandler.KyBdF2 = DefKbHit;
        break;
    case KEYBOARD_F3_KEY:
        GlobalKeyHandler.KyBdF3 = DefKbHit;
        break;
    case KEYBOARD_F4_KEY:
        GlobalKeyHandler.KyBdF4 = DefKbHit;
        break;
    case KEYBOARD_F5_KEY:
        GlobalKeyHandler.KyBdF5 = DefKbHit;
        break;
    case KEYBOARD_F6_KEY:
        GlobalKeyHandler.KyBdF6 = DefKbHit;
        break;
    case KEYBOARD_F7_KEY:
        GlobalKeyHandler.KyBdF7 = DefKbHit;
        break;
    case KEYBOARD_F8_KEY:
        GlobalKeyHandler.KyBdF8 = DefKbHit;
        break;
    case KEYBOARD_F9_KEY:
        GlobalKeyHandler.KyBdF9 = DefKbHit;
        break;
    case KEYBOARD_F10_KEY:
        GlobalKeyHandler.KyBdF10 = DefKbHit;
        break;
    case KEYBOARD_F12_KEY:
        GlobalKeyHandler.KyBdF12 = DefKbHit;
        break;
    case KEYBOARD_UP:
        GlobalKeyHandler.KyBdUp = DefKbHit;
        break;
    case KEYBOARD_DOWN:
        GlobalKeyHandler.KyBdDown = DefKbHit;
        break;
    case KEYBOARD_LEFT:
        GlobalKeyHandler.KyBdLeft = DefKbHit;
        break;
    case KEYBOARD_RIGHT:
        GlobalKeyHandler.KyBdRight = DefKbHit;
        break;
    case KEYBOARD_DEL:
        GlobalKeyHandler.KyBdDel = DefKbHit;
        break;
    case KEYBOARD_INSERT:
        GlobalKeyHandler.KyBdInsert = DefKbHit;
        break;
    case KEYBOARD_END:
        GlobalKeyHandler.KyBdEnd = DefKbHit;
        break;
    case KEYBOARD_HOME:
        GlobalKeyHandler.KyBdHome = DefKbHit;
        break;
    case KEYBOARD_PGUP:
        GlobalKeyHandler.KyBdPgUp = DefKbHit;
        break;
    case KEYBOARD_PGDWN:
        GlobalKeyHandler.KyBdPgDwn = DefKbHit;
        break;
    case KEYBOARD_ESC:
        GlobalKeyHandler.KyBdEsc = DefKbHit;
        break;
    default:
        return -1;
    }
    return 0;
}