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
uint32_t ValidateGroupId(uint32_t group_id);
uint32_t ValidateUserId(uint32_t user_id);