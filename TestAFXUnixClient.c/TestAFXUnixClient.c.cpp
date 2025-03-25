#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define FD_SETSIZE 2
#include <WinSock2.h>
#include <afunix.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_SOCKET "c:\\windows\\temp\\server.sock"

#define MESSAGE "Reply to AFXUNIX!"

#define BUFFER_SIZE 1024
#define recvBufferSize BUFFER_SIZE

char std_input_buffer[BUFFER_SIZE];
int std_input_pos = 0;
char errorbuff[BUFFER_SIZE];

int main()
{
    enum server_state{FirstPacket,ReceivedHello,Closing};
    int state = FirstPacket;
    WSADATA wsadata;
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKADDR_UN sock_addr;
    int rc = 0;
    char recvBuffer[recvBufferSize];
    char* buff = (char*)MESSAGE;

    fpos_t std_in_pos = 0;
    fpos_t std_last_in_pos = 0;

    fd_set read_fds;
    fd_set write_fds;
    fd_set exception_fds;
    
    DWORD one = 1;
    DWORD buffer_ready = 0;
    int sel = 0;

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

    if (ioctlsocket(serverSocket, FIONBIO, &one) < 0)
    {
        fprintf(stderr, "Failed to set non blocking on socket: %d\n", WSAGetLastError());
    }
    sock_addr.sun_family = AF_UNIX;
    strncpy_s(sock_addr.sun_path, sizeof(sock_addr.sun_path), SERVER_SOCKET, sizeof(SERVER_SOCKET) - 1);

    rc = connect(serverSocket, (struct sockaddr*)&sock_addr, sizeof(SOCKADDR_UN));
    if (rc == SOCKET_ERROR && WSAGetLastError()!=WSAEWOULDBLOCK)
    {
        fprintf(stderr, "Failed to connect to the socket:%d\n", WSAGetLastError());
        goto CleanUp;
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&exception_fds);

    

    while (1)
    {
        FD_SET(serverSocket, &read_fds);
        FD_SET(serverSocket, &write_fds);
        FD_SET(serverSocket, &exception_fds);
        // FD_SET(STD_INPUT_HANDLE, &read_fds);
        // FD_SET(STD_OUTPUT_HANDLE, &write_fds);

        sel = select(3, &read_fds, &write_fds, &exception_fds, NULL);
        if (sel < 0)
        {
            continue;
        }
        if (FD_ISSET(serverSocket, &read_fds))
        {
            rc = recv(serverSocket, recvBuffer, recvBufferSize, 0);
            if (rc < 0)
            {
                fprintf(stderr, "Failed to receive:%d\n", WSAGetLastError());
                goto CleanUp;
            }
            if (rc == 0)
            {
                fprintf(stdout,"Socket Closed from server\n");
                goto CleanUp;
            }
            fprintf(stdout, "Received %d bytes: %.*s\n", rc, rc, recvBuffer);
            if (state == FirstPacket)
            {
                if (memcmp(recvBuffer, "Hello",rc) == 0)
                {
                    state = ReceivedHello;
                    fprintf(stdout, "Received Hello\n");
                }
                else
                {
                    fprintf(stderr, "Failed to received Hello:%.*s\n", rc, recvBuffer);
                    goto CleanUp;
                }
            }
            else
            {
                if (memcmp(recvBuffer, "quit",min(rc,strlen("quit"))) == 0)
                {
                    fprintf(stdout,"Remote Server quit\n");
                    goto CleanUp;
                }
            }
        }
        if (_kbhit())
        {

            std_input_buffer[std_input_pos] = _getche();
            if (std_input_buffer[std_input_pos] == '\n'|| std_input_buffer[std_input_pos] == '\r')
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
        if (FD_ISSET(serverSocket, &write_fds) && buffer_ready==1)
        {
            buff = (char*)malloc(std_input_pos + 1);
            if (!buff)
            {
                fprintf(stderr, "Failed to allocated buffer\n");
                goto CleanUp;
            }
            memcpy(buff, std_input_buffer, std_input_pos);
            rc = send(serverSocket, buff, std_input_pos, 0);
            if (rc != SOCKET_ERROR)
            {
                fprintf(stdout, "Sent: %d bytes over the channel\n", rc);
            }
            buffer_ready = 0;
            std_input_pos = 0;
            if (!buff)
            {
                free(buff);
                buff = NULL;
            }
        }
        if (FD_ISSET(serverSocket, &exception_fds))
        {
            fprintf(stderr, "Exception on serversocket\n");
            goto CleanUp;
        }
    }

CleanUp:

    if (serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
    }

    WSACleanup();
    return 0;
}
