#pragma once

#include <stdint.h>

struct group {
    char* groupname;
    uint32_t groupid;
};


wchar_t* GetUserNameFunction();
uint32_t GetUserIdFunction();
struct group* GetGroupFunction(const char* groupName);
int UserIsInGroup(uint32_t uid, uint32_t gid);