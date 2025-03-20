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
    LPBYTE groupBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD_PTR resumeHandle = 0;
    DWORD index = 0;
    LOCALGROUP_INFO_0* lpGroupInfo=NULL;
    if (NetLocalGroupEnum(NULL, 0, &groupBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
    {
        lpGroupInfo = (LOCALGROUP_INFO_0*)groupBuffer;
        for (index = 0; index < numberOfEntries; index++)
        {
            fwprintf(stdout, L"Group Name:%s\n", lpGroupInfo[index].lgrpi0_name);
        }
        NetApiBufferFree(groupBuffer);
    }
    else
    {
        fprintf(stderr,"Failed to get local groups: %d\n", GetLastError());
    }

}
