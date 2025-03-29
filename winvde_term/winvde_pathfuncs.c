#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "winvde_pathfuncs.h"

#define MAXSYMLINKS 10


char* termGetRealPath(const char* name, char* resolved)
{
    char* dest = NULL;
    char* buf = NULL;
    char* extra_buf = NULL;
    const char* start = NULL;
    const char* end = NULL;
    const char* resolved_limit;
    char* resolved_ptr = NULL;
    char* resolved_root = NULL;
    char* ret_path = NULL;
    int num_links = 0;
    struct winvde_stat pst;
    size_t n = 0;
    if (!name || !resolved)
    {
        errno = EINVAL;
        goto abort;
    }
    resolved_ptr = resolved;
    resolved_root = resolved_ptr + 1;
    if (name[0] == '\0')
    {
        /* As per Single Unix Specification V2 we must return an error if
           the name argument points to an empty string.  */
        errno = ENOENT;
        goto abort;
    }

    if ((buf = (char*)malloc(MAX_PATH + 1)) == NULL) {
        errno = ENOMEM;
        goto abort;
    }

    if ((extra_buf = (char*)malloc(MAX_PATH + 1)) == NULL) {
        errno = ENOMEM;
        goto abort;
    }

    resolved_limit = resolved_ptr + MAX_PATH;

    /* relative path, the first char is not '\\' */
    if (!strstr(name, "C:\\") && !strstr(name, "\\"))
    {
        if (!_getcwd(resolved, MAX_PATH))
        {
            resolved[0] = '\0';
            goto abort;
        }

        dest = strchr(resolved, '\0');
    }
    else
    {
        /* absolute path */
        dest = resolved_root;
        resolved[0] = '\\';

        /* special case "/" */
        if (name[1] == 0)
        {
            *dest = '\0';
            ret_path = resolved;
            goto cleanup;
        }
    }

    /* now resolved is the current wd or "/", navigate through the path */
    for (start = end = name; *start; start = end)
    {
        n = 0;
        if (strstr(start + 3, ":\\"))
        {
            start += 6;
        }
        if (strstr(start + 1, ":\\"))
        {
            start += 3;
        }
        /* Skip sequence of multiple path-separators.  */
        while (*start == '\\')
            ++start;



        /* Find end of path component.  */
        for (end = start; *end && *end != '\\'; ++end);

        if (end - start == 0)
            break;
        else if (end - start == 1 && start[0] == '.')
            /* nothing */;
        else if (end - start == 2 && start[0] == '.' && start[1] == '.')
        {
            /* Back up to previous component, ignore if at root already.  */
            if (dest > resolved_root)
                while ((--dest)[-1] != '\\');
        }
        else
        {
            if (dest[-1] != '\\')
                *dest++ = '\\';

            if (dest + (end - start) >= resolved_limit)
            {
                errno = ENAMETOOLONG;
                if (dest > resolved_root)
                    dest--;
                *dest = '\0';
                goto abort;
            }

            /* copy the component, don't use mempcpy for better portability */
            dest = (char*)memcpy(dest, start, end - start) + (end - start);
            *dest = '\0';

            /*check the dir along the path */
            if (termStat(resolved, &pst) < 0)
                goto abort;
            else
            {
                /* this is a symbolic link, thus restart the navigation from
                 * the symlink location */
                if (pst.st_mode & WV_S_IFLNK)
                {
                    size_t len;

                    if (++num_links > MAXSYMLINKS)
                    {
                        errno = ELOOP;
                        goto abort;
                    }

                    /* symlink! */
                    n = termReadLink(resolved_ptr, buf, MAX_PATH);
                    resolved_ptr = buf;
                    if (n == 0xFFFFFFFFFFFFFFFF)
                        goto abort;

                    buf[n] = '\0';

                    len = strlen(end);
                    if ((long)(n + len) >= MAX_PATH)
                    {
                        errno = ENAMETOOLONG;
                        goto abort;
                    }

                    /* Careful here, end may be a pointer into extra_buf... */
                    memmove(&extra_buf[n], end, len + 1);
                    name = end = memcpy(extra_buf, buf, n);

                    if (strstr(&buf[0], "\\\\?\\C:\\"))
                    {
                        dest = resolved_root + 2;
                    }
                    else if (buf[0] == '\\')
                    {
                        dest = resolved_root;	/* It's an absolute symlink */
                    }
                    else if (strstr(&buf[1], ":\\"))
                    {
                        dest = resolved_root + 2;
                    }

                    else
                        /* Back up to previous component, ignore if at root already: */
                        if (dest > resolved + 1)
                            while ((--dest)[-1] != '\\');
                }
                else if (*end == '\\' && (pst.st_mode & S_IFDIR) == 0)
                {
                    errno = ENOTDIR;
                    goto abort;
                }
                else if (*end == '\\')
                {
                    if (_access(resolved, R_OK) != 0)
                    {
                        errno = EACCES;
                        goto abort;
                    }
                }
            }
        }
    }
    if (dest > resolved + 1 && dest[-1] == '/')
        --dest;
    *dest = '\0';

    ret_path = resolved_ptr;
    goto cleanup;

abort:
    ret_path = NULL;
cleanup:
    if (buf) free(buf);
    if (extra_buf) free(extra_buf);
    return ret_path;
}

int termStat(const char* filename, struct winvde_stat* stat)
{
    int rc = 0;
    DWORD dwAttributes = 0;
    struct _stat64 stat_real;
    if (!filename || !stat)
    {
        errno = EINVAL;
        return -1;
    }
    rc = _stat64(filename, &stat_real);
    if (rc == 0)
    {
        stat->st_atime = stat_real.st_atime;
        stat->st_ctime = stat_real.st_ctime;
        stat->st_mtime = stat_real.st_mtime;
        stat->st_dev = stat_real.st_dev;
        stat->st_gid = stat_real.st_gid;
        stat->st_ino = stat_real.st_ino;
        stat->st_mode = stat_real.st_mode;
        stat->st_nlink = stat_real.st_nlink;
        stat->st_rdev = stat_real.st_rdev;
        stat->st_size = stat_real.st_size;
        stat->st_uid = stat_real.st_uid;
        dwAttributes = GetFileAttributesA(filename);
        if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        {
            rc = -1;
        }
        else if (dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            stat->st_mode = stat_real.st_mode | WV_S_IFLNK;
        }
    }
    return rc;
}

size_t termReadLink(const char* path, char* buffer, size_t bufsize)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD result = 0;
    char* file = NULL;
    if (!path || !buffer || bufsize == 0)
    {
        errno = EINVAL;
        return -1;
    }
    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        result = GetFinalPathNameByHandleA(hFile, buffer, (DWORD)bufsize, FILE_NAME_NORMALIZED);
        if (result == 0 || result > bufsize)
        {
            CloseHandle(hFile);
            fprintf(stderr, "Failed to get the full path name: %d\n", GetLastError());
            return -1;
        }
        CloseHandle(hFile);
    }
    return result;
}
