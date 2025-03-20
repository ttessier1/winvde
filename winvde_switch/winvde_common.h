#pragma once

#include <sys/stat.h>

#define WV_S_IFMT   0x1F000 // File type mask
#define WV_S_IFLNK  0x10000 // is link
#define WV_S_IFDIR  0x04000 // Directory
#define WV_S_IFCHR  0x02000 // Character special
#define WV_S_IFIFO  0x01000 // Pipe
#define WV_S_IFREG  0x08000 // Regular
#define WV_S_IREAD  0x00100 // Read permission, owner
#define WV_S_IWRITE 0x00080 // Write permission, owner
#define WV_S_IEXEC  0x00040 // Execute/search permission, owner

struct winvde_stat {
    _dev_t         st_dev;
    _ino_t         st_ino;
    uint32_t       st_mode;
    short          st_nlink;
    short          st_uid;
    short          st_gid;
    _dev_t         st_rdev;
    __int64        st_size;
    __time64_t     st_atime;
    __time64_t     st_mtime;
    __time64_t     st_ctime;
};

char* winvde_realpath(const char* name, char* resolved);

int winvde_stat(const char* filename, struct winvde_stat* stat);
size_t winvde_readlink(const char* path, char* buf, size_t bufsize);