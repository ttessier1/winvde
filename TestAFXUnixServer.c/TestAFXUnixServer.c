#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define FD_SETSIZE 2
#include <WinSock2.h>
#include <afunix.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <conio.h>
#pragma comment (lib,"ws2_32.lib")

#define SERVER_SOCKET "c:\\windows\\temp\\server.sock"
#define message "Message From AFUNIX!"
#define BUFFER_SIZE 1024
char std_input_buffer[BUFFER_SIZE];
int std_input_pos = 0;
char errorbuff[BUFFER_SIZE];



DWORD APIENTRY ClientThread(LPVOID parameter);

DWORD keepRunning = 1;

int main()
{
    WSADATA wsadata;
    
    SOCKET serverSock = INVALID_SOCKET;
    SOCKET clientSock = INVALID_SOCKET;
    SOCKADDR_UN sock_addr = { 0 };
    DWORD one = 1;
    DWORD dwThreadId = 0;
    
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
        strncpy_s((char*) & sock_addr.sun_path, sizeof(sock_addr.sun_path), SERVER_SOCKET, (sizeof(SERVER_SOCKET) - 1));

        rc = remove(SERVER_SOCKET);
        if (rc != 0)
        {
            rc = GetLastError();
            if (rc != ENOENT) {
                fprintf(stderr,"remove() error: %d\n", rc);
                goto CleanUp;
            }
        }

        rc = bind(serverSock, (struct sockaddr*)&sock_addr, sizeof(SOCKADDR_UN));
        if (rc == SOCKET_ERROR)
        {
            fprintf(stderr,"Failed to bind the socket: %d\n", WSAGetLastError());

            goto CleanUp;
        }
        rc = listen(serverSock, SOMAXCONN);
        if (rc == SOCKET_ERROR)
        {
            fprintf(stderr,"Failed to listen the socket: %d\n", WSAGetLastError());
            goto CleanUp;
        }

        if (ioctlsocket(serverSock, FIONBIO, &one) < 0)
        {
            fprintf(stderr, "Failed to set non-blocking: %d\n", WSAGetLastError());
        }
        else
        {
            fprintf(stdout, "Socket is not non-blocking\n");
        }

        while (keepRunning==1)
        {
            errno = 0;
            clientSock = accept(serverSock, NULL, NULL);
            if (clientSock == INVALID_SOCKET && WSAGetLastError() != WSAEWOULDBLOCK)
            {
                fprintf(stderr,"Failed to accept the socket: %d %d\n", errno, WSAGetLastError());
                goto CleanUp;
            }
            else if (clientSock != INVALID_SOCKET)
            {
                fprintf(stdout, "Accepted the connection\n");

                CreateThread(NULL, 0, ClientThread, (LPVOID)clientSock, 0, &dwThreadId);
            }
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

DWORD APIENTRY ClientThread(LPVOID parameter)
{
    enum server_state {
        FirstPacket,
        HelloSent,
        Closing
    };
    SOCKET clientSocket = INVALID_SOCKET;
    LPWSAPOLLFD wsaPollFD =NULL;

    int state = FirstPacket;
    char * buff = NULL;
    char * out = NULL;

    int buffer_ready = 0;

    int bytesRead = 0;
    int bytesWritten = 0;

    

    int sel = 0;
    if (parameter)
    {
        clientSocket = (SOCKET)parameter;
        if (clientSocket == INVALID_SOCKET)
        {
            fprintf(stderr, "Invalid Socket sent to thread\n");
            goto CleanUp;
        }

        wsaPollFD = (WSAPOLLFD*)malloc(sizeof(WSAPOLLFD) * 2);
        if (!wsaPollFD)
        {
            fprintf(stderr, "Failed to allocate memory for PollFD\n");
            goto CleanUp;
        }
        wsaPollFD[0].fd = clientSocket;
        wsaPollFD[0].events = POLLIN ;
        wsaPollFD[0].revents = 0;
        wsaPollFD[1].fd = clientSocket;
        wsaPollFD[1].events = POLLOUT;
        wsaPollFD[1].revents = 0;

        buff = (char*)malloc(1500);
        if (!buff)
        {
            errno = ENOMEM;
            closesocket(clientSocket);
            return -1;
        }

        
        while (1)
        {

            sel = WSAPoll(wsaPollFD, 2, 0);

            if (sel < 0)
            {
                fprintf(stderr, "Failed to WsaPoll: %d\n",WSAGetLastError());
                goto CleanUp;
            }

            if (wsaPollFD[0].revents&POLLIN)
            {

                memset(buff, 0, sizeof(buff));

                bytesRead = recv(clientSocket, buff, sizeof(buff), 0);

                if (bytesRead < 0)
                {
                    fprintf(stderr, "Failed receive: %d\n", WSAGetLastError());
                    goto CleanUp;
                }
                else if (bytesRead == 0)
                {
                    fprintf(stderr, "Connection Closed\n");
                    goto CleanUp;
                }
                else
                {
                    if (memcmp(buff, "quit",min(bytesRead,strlen("quit"))) == 0)
                    {
                        fprintf(stdout, "Received quit message\n");
                        goto CleanUp;
                    }
                    if (memcmp(buff, "exit", min(bytesRead, strlen("quit"))) == 0)
                    {
                        fprintf(stdout, "Received exit message\n");
                        goto CleanUp;
                    }
                    fprintf(stdout, "Client Write: %d bytes %.*s\n", bytesRead, bytesRead, buff);
                }
            }
            if (wsaPollFD[0].revents & POLLHUP)
            {
                fprintf(stderr, "Remote Hangup\n");
                break;
            }
            if (_kbhit())
            {

                std_input_buffer[std_input_pos] = _getche();
                if (std_input_buffer[std_input_pos] == '\n' || std_input_buffer[std_input_pos] == '\r')
                {

                    buffer_ready = 1;
                }
                else if (std_input_pos >= BUFFER_SIZE - 1)
                {
                    buffer_ready = 1;
                }
                else
                {

                    std_input_pos++;
                }

            }

           
            if (wsaPollFD[1].revents & POLLOUT &&buffer_ready==1 ||
                wsaPollFD[1].revents & POLLOUT && state==FirstPacket
            )
            {
                if (state == FirstPacket)
                {
                    send(clientSocket, (char*)"Hello", (int)strlen("Hello"), 0);
                    state = HelloSent;
                }
                else
                {
                    out = (char*)malloc(std_input_pos+1);
                    if (!out)
                    {
                        fprintf(stderr, "Failed to allocate memory for buffer\n");
                        goto CleanUp;
                    }
                    memcpy(out, std_input_buffer, std_input_pos);
                    if (memcmp(std_input_buffer, "exit", min(std_input_pos, strlen("exit")))==0)
                    {
                        
                        send(clientSocket, out, std_input_pos, 0);
                        bytesWritten = 0;
                        buffer_ready = 0;
                        std_input_pos = 0;
                        if (out)
                        {
                            free(out);
                            out = NULL;
                        }
                        keepRunning = 0;
                        break;
                    }
                    send(clientSocket, out, std_input_pos, 0);
                    bytesWritten = 0;
                    buffer_ready = 0;
                    std_input_pos = 0;
                    if(out)
                    {
                        free(out);
                        out = NULL;
                    }
                }
            }
            if (wsaPollFD[1].revents & POLLHUP)
            {
                fprintf(stderr, "Remote Hangup\n");
                break;
            }
        }
    }
    else
    {
        fprintf(stderr, "Failed to get socket from thread parameter\n");
    }
CleanUp:
    if (out)
    {
        free(out);
        out = NULL;
    }
    if (buff)
    {
        free(buff);
        buff = NULL;
    }
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
    }
    return 0;
}

