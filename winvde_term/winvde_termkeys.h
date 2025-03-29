#pragma once

#include <stdint.h>

#define STD_INPUT_BUFF_SIZE 1024

extern uint16_t std_input_pos ;
extern uint16_t std_input_length;
extern char std_input_buffer[STD_INPUT_BUFF_SIZE];
extern char std_input_cpy_buffer[STD_INPUT_BUFF_SIZE];
extern uint8_t insertMode;
extern uint8_t bufferReady;

//typedef void (__cdecl *keyhandlerfunction)();

#define KEY_MIN 0

#define NUMPAD_DOT_KEY 0
#define NUMPAD_0_KEY 1
#define NUMPAD_UP_KEY 2
#define NUMPAD_DOWN_KEY 3
#define NUMPAD_LEFT_KEY 4
#define NUMPAD_RIGHT_KEY 5
#define NUMPAD_HOME_KEY 6
#define NUMPAD_END_KEY 7
#define NUMPAD_PGUP_KEY 8
#define NUMPAD_PGDOWN_KEY 9
#define KEYBOARD_F1_KEY 10
#define KEYBOARD_F2_KEY 11
#define KEYBOARD_F3_KEY 12
#define KEYBOARD_F4_KEY 13
#define KEYBOARD_F5_KEY 14
#define KEYBOARD_F6_KEY 15
#define KEYBOARD_F7_KEY 16
#define KEYBOARD_F8_KEY 17
#define KEYBOARD_F9_KEY 18
#define KEYBOARD_F10_KEY 19
#define KEYBOARD_F12_KEY 20

#define KEYBOARD_UP 21
#define KEYBOARD_DOWN 22
#define KEYBOARD_LEFT 23
#define KEYBOARD_RIGHT 24
#define KEYBOARD_DEL 25
#define KEYBOARD_INSERT 26
#define KEYBOARD_END 27
#define KEYBOARD_HOME 28
#define KEYBOARD_PGUP 29
#define KEYBOARD_PGDWN 30
#define KEYBOARD_ESC 31
#define KEY_MAX 31

struct key_handlers{
    void(*NumPadDot)();
    void(*NumPadZero)();
    void(*NumPadUp)();
    void(*NumPadDown)();
    void(*NumPadLeft)();
    void(*NumPadRight)();
    void(*NumPadHome)();
    void(*NumPadEnd)();
    void(*NumPadPgUp)();
    void(*NumPadPgDwn)();
    void(*KyBdF1)();
    void(*KyBdF2)();
    void(*KyBdF3)();
    void(*KyBdF4)();
    void(*KyBdF5)();
    void(*KyBdF6)();
    void(*KyBdF7)();
    void(*KyBdF8)();
    void(*KyBdF9)();
    void(*KyBdF10)();
    void(*KyBdF12)();
    void(*KyBdUp)();
    void(*KyBdDown)();
    void(*KyBdLeft)();
    void(*KyBdRight)();
    void(*KyBdDel)();
    void(*KyBdInsert)();
    void(*KyBdEnd)();
    void(*KyBdHome)();
    void(*KyBdPgUp)();
    void(*KyBdPgDwn)();
    void(*KyBdEsc)();
};

extern struct key_handlers GlobalKeyHandler;

void DefKbHit();
int AttachKeyHandler(int32_t  key, void(*func)());
int RemoveKeyHandler(uint8_t key, void(*func)());
int DoKbHit();