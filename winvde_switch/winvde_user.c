#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <lm.h>
#include <Lmcons.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#pragma comment(lib,"Netapi32.lib")

#include "winvde_user.h"
#include "winvde_mgmt.h"


wchar_t * GetUserNameFunction()
{
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;

    if (GetUserName(username, &username_len)) {
        fprintf(stdout,"Currently logged on user: %S\n", username );
    }
    else
    {
        fprintf(stderr, "Failed to get the username. Error code: %d\n", GetLastError());
    }
    return _wcsdup(username);
}

uint32_t ValidateUserId(uint32_t user_id)
{
    LPBYTE userBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD resumeHandle = 0;
    DWORD index = -1;
    DWORD found = 0;
    USER_INFO_0* lpUserInfo = NULL;

    if (user_id >=0)
    {
        if (user_id == 0)
        {
            return 0;
        }
        else
        {
            if (NetUserEnum(NULL, 0, FILTER_NORMAL_ACCOUNT, &userBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
            {
                lpUserInfo = (USER_INFO_0*)userBuffer;
                for (index = 1; index < numberOfEntries; index++)
                {
                    if (user_id == index)
                    {
                        found = 1;
                        break;
                    }

                }
                NetApiBufferFree(userBuffer);
            }
            else
            {
                fprintf(stderr, "Failed to get local groups: %d\n", GetLastError());
            }
        }
    }
    if (found == 1)
    {
        return index;
    }
    else
    {
        return -1;
    }

}

uint32_t GetUserIdFunction()
{
    LPBYTE userBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD resumeHandle = 0;
    DWORD index = -1;
    DWORD found = 0;
    USER_INFO_0* lpUserInfo = NULL;
    wchar_t*  username = GetUserNameFunction();
    if (username != NULL)
    {
        if (wcscmp(username, L"Administrator") == 0)
        {
            return 0;
        }
        else
        {
            if (NetUserEnum(NULL, 0, FILTER_NORMAL_ACCOUNT, &userBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
            {
                lpUserInfo = (USER_INFO_0*)userBuffer;
                for (index = 1; index < numberOfEntries; index++)
                {
                    if (wcscmp(lpUserInfo[index].usri0_name, L"Administrator") == 0
                        &&
                        wcscmp(lpUserInfo[index].usri0_name, username) == 0)
                    {
                        index = 0;
                        found = 1;
                        break;
                    }
                    else if (wcscmp(lpUserInfo[index].usri0_name, username) == 0 )
                    {
                        //fwprintf(stdout, L"User Name:%s\n", lpUserInfo[index].usri0_name);
                        found = 1;
                        break;
                    }
                    
                }
                NetApiBufferFree(userBuffer);
            }
            else
            {
                fprintf(stderr, "Failed to get local groups: %d\n", GetLastError());
            }
        }
        free(username);
    }
    if (found == 1)
    {
        return index;
    }
    else
    {
        return -1;
    }
    
}

uint32_t ValidateGroupId(uint32_t group_id)
{
    struct group* theGroup = NULL;
    LPBYTE groupBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD_PTR resumeHandle = 0;
    DWORD index = 0;
    DWORD found = 0;
    LOCALGROUP_INFO_0* lpGroupInfo = NULL;
    size_t nameLength = 0;
    wchar_t* wideGroupName = NULL;
    if (group_id >= 0)
    {
        if (group_id == 0)
        {
            return 0;
        }
        if (NetLocalGroupEnum(NULL, 0, &groupBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
        {
            lpGroupInfo = (LOCALGROUP_INFO_0*)groupBuffer;
            for (index = 0; index < numberOfEntries; index++)
            {
                if (index == group_id)
                {
                    found = 1;
                    break;
                }
            }
            NetApiBufferFree(groupBuffer);
        }
        else
        {
            fprintf(stderr, "Failed to get local groups: %d\n", GetLastError());
        }
    }
    if (found == 1)
    {
        return index;
    }
    else
    {
        return -1;
    }
}

struct group * GetGroupFunction(const char * groupName)
{
    struct group* theGroup = NULL;
    LPBYTE groupBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD_PTR resumeHandle = 0;
    DWORD index = 0;
    LOCALGROUP_INFO_0* lpGroupInfo = NULL;
    size_t nameLength = 0;
    wchar_t* wideGroupName = NULL;

  
    if (!groupName)
    {
        switch_errno = EINVAL;
        return NULL;
    }
    nameLength = strlen(groupName);
    wideGroupName = malloc(sizeof(wchar_t) * (nameLength + 1));
    if (!wideGroupName)
    {
        switch_errno = ENOMEM;
        return NULL;
    }
    mbtowc(wideGroupName, groupName, nameLength);
    if (NetLocalGroupEnum(NULL, 0, &groupBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
    {
        lpGroupInfo = (LOCALGROUP_INFO_0*)groupBuffer;
        for (index = 0; index < numberOfEntries; index++)
        {
            fwprintf(stdout, L"Group Name:%s\n", lpGroupInfo[index].lgrpi0_name);
            if (wcscmp(wideGroupName, lpGroupInfo[index].lgrpi0_name)==0)
            {
                theGroup = (struct group*)malloc(sizeof(struct group));
                if (theGroup)
                {
                    theGroup->groupid = index;
                    theGroup->groupname = (char*)groupName;
                }
                break;
            }
        }
        NetApiBufferFree(groupBuffer);
    }
    else
    {
        fprintf(stderr, "Failed to get local groups: %d\n", GetLastError());
    }
    if (wideGroupName)
    {
        free(wideGroupName);
        wideGroupName = NULL;
    }
    return theGroup;
}

struct group* GetGroupFunctionW(const wchar_t* groupName)
{
    struct group* theGroup = NULL;
    LPBYTE groupBuffer = NULL;
    DWORD numberOfEntries = 0;
    DWORD totalNumberOfEntries = 0;
    DWORD_PTR resumeHandle = 0;
    DWORD index = 0;
    LOCALGROUP_INFO_0* lpGroupInfo = NULL;
    size_t nameLength = 0;
    wchar_t* wideGroupName = NULL;


    if (!groupName)
    {
        switch_errno = EINVAL;
        return NULL;
    }
    nameLength = wcslen(groupName);

    
    if (NetLocalGroupEnum(NULL, 0, &groupBuffer, MAX_PREFERRED_LENGTH, &numberOfEntries, &totalNumberOfEntries, &resumeHandle) == NERR_Success)
    {
        lpGroupInfo = (LOCALGROUP_INFO_0*)groupBuffer;
        for (index = 0; index < numberOfEntries; index++)
        {
            fwprintf(stdout, L"Group Name:%s\n", lpGroupInfo[index].lgrpi0_name);
            if (wcscmp(groupName, lpGroupInfo[index].lgrpi0_name) == 0)
            {
                theGroup = (struct group*)malloc(sizeof(struct group));
                if (theGroup)
                {
                    theGroup->groupid = index;
                    theGroup->groupname = (char*)groupName;
                }
                break;
            }
        }
        NetApiBufferFree(groupBuffer);
    }
    else
    {
        fprintf(stderr, "Failed to get local groups: %d\n", GetLastError());
    }
    if (wideGroupName)
    {
        free(wideGroupName);
        wideGroupName = NULL;
    }
    return theGroup;
}

#ifdef VDE_BIONIC
int UserIsInGroup(uid_t uid, gid_t gid) { return 0; }
#else
/* 1 if user belongs to the group, 0 otherwise) */
int UserIsInGroup(uint32_t uid, uint32_t gid)
{
    DWORD entries = 0;
    DWORD totalEntries = 0;
    struct group* theGroup = NULL;
    wchar_t* username = GetUserNameFunction();
    LPBYTE buffer= NULL;
    LOCALGROUP_USERS_INFO_0* lpGroups = NULL;

    int returnCode = 0;
    if (username != NULL)
    {
        if (NetUserGetLocalGroups(NULL, username, 0, 0, &buffer, MAX_PREFERRED_LENGTH, &entries, &totalEntries) == ERROR_SUCCESS)
        {
            lpGroups = (LOCALGROUP_USERS_INFO_0*)buffer;
            theGroup = GetGroupFunctionW(lpGroups->lgrui0_name);
            if (theGroup != NULL)
            {
                if (gid == theGroup->groupid)
                {
                    returnCode = 1;
                }
                else
                {
                    returnCode = 0;
                }
                free(theGroup);
            }
        }
        free(username);
    }
    return returnCode;
}
#endif
