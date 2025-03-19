#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <afunix.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib,"ws2_32.lib")

#define SERVER_SOCKET "server.sock"
#define message "Message From AFUNIX!"

int main()
{
    WSADATA wsadata;
    
    SOCKET serverSock = INVALID_SOCKET;
    SOCKET clientSock = INVALID_SOCKET;
    SOCKADDR_UN sock_addr = { 0 };

    char* buff = message;
    char* replyBuff = NULL;
    int replyBuffSize = 0;
    int rc=0;

    memset(&wsadata, 0, sizeof(WSADATA));

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR)
    {
        fprintf(stderr, "Failed to start sockets:%d\n", WSAGetLastError());
        exit(0xFFFFFFFF);
    }

    serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSock != INVALID_SOCKET)
    {
        sock_addr.sun_family = AF_UNIX;
        strncpy_s(&sock_addr.sun_path, sizeof(sock_addr.sun_path), SERVER_SOCKET, (sizeof(SERVER_SOCKET) - 1));

        rc = remove(SERVER_SOCKET);
        if (rc != 0)
        {
            rc = WSAGetLastError();
            if (rc != ENOENT) {
                fprintf(stderr,"remove() error: %d\n", rc);
                goto CleanUp;
            }
        }

        rc = bind(serverSock,(struct sockaddr*)&sock_addr,sizeof(SOCKADDR_UN));
        if (rc == SOCKET_ERROR)
        {
            fprintf("Failed to bind the socket: %d\n", WSAGetLastError());
            goto CleanUp;
        }
        rc = listen(serverSock, SOMAXCONN);
        if (rc == SOCKET_ERROR)
        {
            fprintf("Failed to listen the socket: %d\n", WSAGetLastError());
            goto CleanUp;
        }

        clientSock = accept(serverSock, NULL, NULL);
        if(clientSock== INVALID_SOCKET)
        {
            fprintf("Failed to accept the socket: %d\n", WSAGetLastError());
            goto CleanUp;
        }
        fprintf(stdout, "Accepted the connection\n");

        rc = send(clientSock, (const char*)buff, strlen(buff), 0);
        if(rc ==SOCKET_ERROR)
        {
            printf("send() error: %d\n", WSAGetLastError());
            goto CleanUp;
        }

        fprintf(stdout, "Relayed: %d bytes\n",rc);
        replyBuffSize = 1500;
        replyBuff = (char*)malloc(replyBuffSize);
        if (replyBuff)
        {
            rc = recv(clientSock, replyBuff, replyBuffSize, 0);
            if (rc != SOCKET_ERROR)
            {
                fprintf(stdout, "Received %d bytes:%.*s", rc,rc, replyBuff);
            }
            
            free(replyBuff);
        }


        
    }
    else
    {
        fprintf(stderr, "Failed to create unix socket:%d\n", WSAGetLastError());
    }

CleanUp:
    if (serverSock)
    {
        closesocket(serverSock);
    }
    if (clientSock)
    {
        closesocket(clientSock);
    }
    DeleteFileA(SERVER_SOCKET);
    WSACleanup();
}