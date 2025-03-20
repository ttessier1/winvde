#pragma once

struct group {
    char* groupname;
    uint32_t groupid;
};


wchar_t* GetUserNameFunction();
uint32_t GetUserIdFunction();
struct group* GetGroupFunction(const char* groupName);