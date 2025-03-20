// TestWinStat.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fileapi.h>
#include <sys\stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <direct.h>

char errorbuff[1024];

#define _S_IFMT   0xF000 // File type mask
#define _S_IFDIR  0x4000 // Directory
#define _S_IFCHR  0x2000 // Character special
#define _S_IFIFO  0x1000 // Pipe
#define _S_IFREG  0x8000 // Regular
#define _S_IREAD  0x0100 // Read permission, owner
#define _S_IWRITE 0x0080 // Write permission, owner
#define _S_IEXEC  0x0040 // Execute/search permission, owner

int main()
{
    struct _stat64 st;

    if (_stat64("src2", &st) < 0)
    {
        strerror_s(errorbuff, sizeof(errorbuff),errno);
        fprintf(stderr, "Failed to stat file: %s\n", errorbuff);
        exit(0xFFFFFFF);
    }
    fprintf(stdout, "Link Mode: %08x\n",st.st_mode);
    if (st.st_mode & _S_IFREG)
    {
        fprintf(stdout, "Is Regular File\n");
    }
    if (st.st_mode & _S_IFDIR)
    {
        fprintf(stdout, "Is Directory File\n");
    }
    if (st.st_mode & _S_IFCHR)
    {
        fprintf(stdout, "Is Special File\n");
    }
    if (st.st_mode & _S_IFIFO)
    {
        fprintf(stdout, "Is Fifo File\n");
    }
    if (st.st_mode & _S_IREAD)
    {
        fprintf(stdout, "Is Read File\n");
    }
    if (st.st_mode & _S_IWRITE)
    {
        fprintf(stdout, "Is Write File\n");
    }
    if (st.st_mode & _S_IEXEC)
    {
        fprintf(stdout, "Is Exec File\n");
    }
    fprintf(stdout, "%d Number of Links\n",st.st_nlink);

    if (_stat64("winvdeplug", &st) < 0)
    {
        strerror_s(errorbuff, sizeof(errorbuff), errno);
        fprintf(stderr, "Failed to stat file: %s\n", errorbuff);
        exit(0xFFFFFFF);
    }
    fprintf(stdout, "Link Mode: %08x\n", st.st_mode);
    if (st.st_mode & _S_IFREG)
    {
        fprintf(stdout, "Is Regular File\n");
    }
    if (st.st_mode & _S_IFDIR)
    {
        fprintf(stdout, "Is Directory File\n");
    }
    if (st.st_mode & _S_IFCHR)
    {
        fprintf(stdout, "Is Special File\n");
    }
    if (st.st_mode & _S_IFIFO)
    {
        fprintf(stdout, "Is Fifo File\n");
    }
    if (st.st_mode & _S_IREAD)
    {
        fprintf(stdout, "Is Read File\n");
    }
    if (st.st_mode & _S_IWRITE)
    {
        fprintf(stdout, "Is Write File\n");
    }
    if (st.st_mode & _S_IEXEC)
    {
        fprintf(stdout, "Is Exec File\n");
    }
    fprintf(stdout, "%d Number of Links\n", st.st_nlink);

    if (_stat64("src3", &st) < 0)
    {
        strerror_s(errorbuff, sizeof(errorbuff), errno);
        fprintf(stderr, "Failed to stat file: %s\n", errorbuff);
        exit(0xFFFFFFF);
    }
    fprintf(stdout, "Link Mode: %08x\n", st.st_mode);
    if (st.st_mode & _S_IFREG)
    {
        fprintf(stdout, "Is Regular File\n");
    }
    if (st.st_mode & _S_IFDIR)
    {
        fprintf(stdout, "Is Directory File\n");
    }
    if (st.st_mode & _S_IFCHR)
    {
        fprintf(stdout, "Is Special File\n");
    }
    if (st.st_mode & _S_IFIFO)
    {
        fprintf(stdout, "Is Fifo File\n");
    }
    if (st.st_mode & _S_IREAD)
    {
        fprintf(stdout, "Is Read File\n");
    }
    if (st.st_mode & _S_IWRITE)
    {
        fprintf(stdout, "Is Write File\n");
    }
    if (st.st_mode & _S_IEXEC)
    {
        fprintf(stdout, "Is Exec File\n");
    }
    fprintf(stdout, "%d Number of Links\n", st.st_nlink);
    //0x41ff;

    if (_stat64("vde", &st) < 0)
    {
        strerror_s(errorbuff, sizeof(errorbuff), errno);
        fprintf(stderr, "Failed to stat file: %s\n", errorbuff);
        exit(0xFFFFFFF);
    }
    fprintf(stdout, "Link Mode: %08x\n", st.st_mode);
    if (st.st_mode & _S_IFREG)
    {
        fprintf(stdout, "Is Regular File\n");
    }
    if (st.st_mode & _S_IFDIR)
    {
        fprintf(stdout, "Is Directory File\n");
    }
    if (st.st_mode & _S_IFCHR)
    {
        fprintf(stdout, "Is Special File\n");
    }
    if (st.st_mode & _S_IFIFO)
    {
        fprintf(stdout, "Is Fifo File\n");
    }
    if (st.st_mode & _S_IREAD)
    {
        fprintf(stdout, "Is Read File\n");
    }
    if (st.st_mode & _S_IWRITE)
    {
        fprintf(stdout, "Is Write File\n");
    }
    if (st.st_mode & _S_IEXEC)
    {
        fprintf(stdout, "Is Exec File\n");
    }
    fprintf(stdout, "%d Number of Links\n", st.st_nlink);
    WIN32_FIND_DATAA finddata;
    HANDLE fileSearch = FindFirstFileA("*.*",&finddata);
    if (fileSearch)
    {
        do
        {
            fprintf(stderr, "%s \n",finddata.cFileName);
            if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                fprintf(stderr, "Directory \n");
            }
            if (finddata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
            {
                fprintf(stderr, "File \n");
            }
            if (finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                fprintf(stderr, "Link \n");
            }
        } while (FindNextFileA(fileSearch, &finddata));
    }

    return 0;
}