#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <afunix.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_SOCKET "server.sock"

#define MESSAGE "Reply to AFXUNIX!"

#define recvBufferSize 1500

int main()
{
    WSADATA wsadata;
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKADDR_UN sock_addr;
    int rc = 0;
    char recvBuffer[recvBufferSize];
    char* buff = (char*)MESSAGE;

    memset(&wsadata, 0, sizeof(WSADATA));


    if (WSAStartup(MAKEWORD(2, 2), &wsadata)==SOCKET_ERROR)
    {
        fprintf(stderr, "Failed to startup sockets:%d\n", WSAGetLastError());
        exit(0xFFFFFFF);
    }

    serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Failed to create the server socket: %d\n", WSAGetLastError());
        goto CleanUp;
    }

    sock_addr.sun_family = AF_UNIX;
    strncpy_s(sock_addr.sun_path, sizeof(sock_addr.sun_path), SERVER_SOCKET, sizeof(SERVER_SOCKET) - 1);

    rc = connect(serverSocket, (struct sockaddr*)&sock_addr, sizeof(SOCKADDR_UN));
    if (rc == SOCKET_ERROR)
    {
        fprintf(stderr, "Failed to connect to the socket:%d\n", WSAGetLastError());
        goto CleanUp;
    }

    rc = recv(serverSocket, recvBuffer, recvBufferSize, 0);
    if (rc < 0)
    {
        fprintf(stderr, "Failed to receive:%d\n", WSAGetLastError());
        goto CleanUp;
    }

    fprintf(stdout, "Received %d bytes: %.*s\n", rc, rc, recvBuffer);

    rc= send(serverSocket, buff, strlen(buff), 0);
    if (rc != SOCKET_ERROR)
    {
        fprintf(stdout, "Sent: %d bytes over the channel\n",rc );
    }

CleanUp:
    if (serverSocket)
    {
        closesocket(serverSocket);
    }

    WSACleanup();
    return 0;
}
