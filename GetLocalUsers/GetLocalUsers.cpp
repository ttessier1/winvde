// GetLocalGroups.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <lm.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib,"Netapi32.lib")

int main()
{
    LPBYTE userBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD resumeHandle = 0;
    DWORD index = 0;
    USER_INFO_0* lpUserInfo = NULL;
    if (NetUserEnum(NULL, 0, FILTER_NORMAL_ACCOUNT , &userBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
    {
        lpUserInfo = (USER_INFO_0*)userBuffer;
        for (index = 0; index < numberOfEntries; index++)
        {
            fwprintf(stdout, L"User Name:%s\n", lpUserInfo[index].usri0_name);
        }
        NetApiBufferFree(userBuffer);
    }
    else
    {
        fprintf(stderr, "Failed to get local groups: %d\n", GetLastError());
    }

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
