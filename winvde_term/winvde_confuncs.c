#include <stdio.h>
#include <stdlib.h>

#include "winvde_termkeys.h"
#include "winvde_confuncs.h"

void MoveCursorPositionLeft()
{
    if (std_input_pos > 0)
    {
        fprintf(stdout, "\033[1D");
        std_input_pos--;
    }
}

void MoveCursorPositionRight()
{
    if (std_input_pos < std_input_length)
    {
        fprintf(stdout, "\033[1C");
        std_input_pos++;
    }
}

void SaveCursorPos()
{
    fprintf(stdout, "\0337");
}

void RestoreCursorPos()
{
    fprintf(stdout, "\0338");
}

void ToggleInsert()
{
    if (insertMode == 0)
    {
        insertMode = 1;
        fprintf(stdout, "\033[1 q");
    }
    else
    {
        insertMode = 0;
        fprintf(stdout, "\033[5 q");
    }
}

void ScrollUp()
{
    fprintf(stdout, "\033[1S");
}

void ScrollDown()
{
    fprintf(stdout, "\033[1T");
}

void SetScrollRegion()
{
    fprintf(stdout, "\033[1;24");
}

void InsertLine()
{
    fprintf(stdout, "\033[1L");
}