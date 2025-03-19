// TestWinVde.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <direct.h>

#include <libwinvde.h>
#include <winvdeplugmodule.h>
#include <network_defines.h>


#pragma comment(lib,"winvde.lib")
#pragma comment(lib,"ws2_32.lib")

#define PROTOCOL_ICMPV4 1
#define PROTOCOL_TCPV4 6
#define PROTOCOL_TELNET 14
#define PROTOCOL_UDP 17

#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACHABLE 3
#define ICMP_SOURCE_QUENCH 4
#define ICMP_REDIRECT 5
#define ICMP_ECHO 8
#define ICMP_ROUTER_ADVERTISEMENT 9
#define ICMP_ROUTER_SOLICITATION 10
#define ICMP_TIME_EXCEEDED 11
#define ICMP_PARAMETER_PROBLEM 12
#define ICMP_TIMESTAMP 13
#define ICMP_TIMESTAMP_REPLY 14
#define ICMP_INFO_REQ   15
#define ICMP_INFO_REQ_RPLY   16

#define ICMP_TRACEROUTE 30

#define ICMP_ADDRES_MASK_REQ 17
#define ICMP_ADDRES_MASK_REPLY 18

#define ICMP_EXTENDED_ECHO_REQ 42
#define ICMP_EXTENDED_ECHO_REPLY 43

#define BUFF_SIZE 1500



uint32_t CalculateCheckSum(char* packet, size_t length);
void OutputCurrentDirectory();

char buff[BUFF_SIZE];

char * alphabet = "abcdefghijklmnopqrstuvwxyz";

int main()
{

    WSADATA wsadata;
    uint8_t index = 0;
    size_t bytes_exchanged = 0;
    struct eth_header* lpEthHeader = NULL;
    struct _ip_header* lpIpHeader = NULL;
    struct _icmp_header* lpIcmpHeader = NULL;
    char* portgroup = (char*)"12345";
    char* group = (char*)"1";
    char* modestr = (char*)"1";
    struct winvde_parameters parameters[] = {
       {(char*)"",&portgroup},
       {(char*)"port", &portgroup},
       {(char*)"portgroup",&portgroup},
       {(char*)"group",&group},
       {(char*)"mode",&modestr},
       {NULL,NULL},
    };
    struct winvde_open_args open_args ;
    WINVDECONN * winvdeconn = NULL;
    WINVDECONN* actualconn = NULL;
    struct winvde_netnode_conn* nodeconn = NULL ;
    time_t timestamp;

    memset(&wsadata, 0, sizeof(WSADATA));
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR)
    {
        fprintf(stderr, "Failed to initialized sockets: %d\n", WSAGetLastError());
    }
    
    OutputCurrentDirectory();

    memset(&open_args, 0, sizeof(struct winvde_open_args));
    winvdeconn= winvde_open_real((char*)"winvde",(char*)"test", LIBWINVDE_PLUG_VERSION,&open_args);
    if (winvdeconn != NULL)
    {
        fprintf(stdout, "Opened winvde\n");
        actualconn  = winvdeconn->module.winvde_open_real((const char*)"winvde", (const char*)"test", LIBWINVDE_PLUG_VERSION, &open_args);
        if (actualconn != NULL)
        {
            nodeconn = (struct winvde_netnode_conn*)actualconn;



            actualconn->module.winvde_open_real = winvdeconn->module.winvde_open_real;
            actualconn->module.winvde_recv = winvdeconn->module.winvde_recv;
            actualconn->module.winvde_send = winvdeconn->module.winvde_send;
            actualconn->module.winvde_ctlfiledescriptor = winvdeconn->module.winvde_ctlfiledescriptor;
            actualconn->module.winvde_datafiledescriptor = winvdeconn->module.winvde_datafiledescriptor;
            actualconn->module.winvde_close = winvdeconn->module.winvde_close;

            

            fprintf(stdout, "Opened actual winvde\n");
            lpEthHeader = (struct eth_header*)&buff[0];
            lpEthHeader->destination[0] = 0xFF;
            lpEthHeader->destination[1] = 0xFF;
            lpEthHeader->destination[2] = 0xFF;
            lpEthHeader->destination[3] = 0xFF;
            lpEthHeader->destination[4] = 0xFF;
            lpEthHeader->destination[5] = 0xFF;

            lpEthHeader->source[0] = 0xFF;
            lpEthHeader->source[1] = 0xFF;
            lpEthHeader->source[2] = 0xFF;
            lpEthHeader->source[3] = 0xFF;
            lpEthHeader->source[4] = 0xFF;
            lpEthHeader->source[5] = 0xFF;

            lpEthHeader->ethtype = 0x800;

            lpIpHeader = (struct _ip_header*)(buff + sizeof(struct _ip_header));
            lpIpHeader->IHL = 5 * 32;
            lpIpHeader->Version = 4;
            lpIpHeader->DestinationAddress = htonl(0x0a0a0afe);
            lpIpHeader->SourceAddress = htonl(0x0a0a0af9);
            lpIpHeader->Protocol = PROTOCOL_ICMPV4; // icmp
            lpIpHeader->TTL = 1;
            lpIpHeader->Fragmentation = 0;
            lpIpHeader->TotalLength = htons(sizeof(struct eth_header) + sizeof(struct _ip_header) + sizeof(struct _icmp_header));
            lpIpHeader->Identification = _getpid();
            lpIpHeader->TypeOfService = 0;
            lpIpHeader->CheckSum = CalculateCheckSum((char*)lpIpHeader, sizeof(struct _ip_header));

            lpIcmpHeader = (struct _icmp_header *)(buff + sizeof(struct eth_header) + sizeof(struct _ip_header));

            lpIcmpHeader->type = ICMP_ECHO;
            lpIcmpHeader->Code = 0;
            lpIcmpHeader->icmp_id = _getpid();
            time(&timestamp);
            lpIcmpHeader->icmp_sequence = (uint16_t)timestamp + 1;

            for (index = 0; index < 32; index++)
            {
                buff[sizeof(struct eth_header) + sizeof(struct _ip_header) + sizeof(struct _icmp_header) + index] = alphabet[index % 26];
            }


        
            bytes_exchanged = winvde_send(actualconn, buff, sizeof(struct eth_header) + sizeof(struct _ip_header) + sizeof(struct _icmp_header) + 32, 0);
            fprintf(stdout, "Bytes Exchanged with winvde_send: %lld\n", bytes_exchanged);

            bytes_exchanged = winvde_recv(actualconn, buff, sizeof(buff), 0);
            fprintf(stdout, "Bytes Exchanged with winvde_recv: %lld\n", bytes_exchanged);
            winvde_close(actualconn);
        }
        else
        {
            fprintf(stderr, "Failed to open actual winvde\n");
        }
        winvde_close(winvdeconn);
    }
    else
    {
        fprintf(stderr, "Failed to open winvde\n");
    }
    WSACleanup();
    return 0;
}

uint32_t CalculateCheckSum(char* packet, size_t length)
{
    uint16_t checksum = 0;
    uint32_t sum = 0;
    char* lpaddr = packet;
    if (packet != NULL)
    {
        while (length > 1)
        {
            sum += ((uint16_t)(*(uint16_t*)lpaddr));
            lpaddr += 2;
            length -= 2;
        }

        if (length > 0)
        {
            sum += *(uint8_t*)lpaddr;
        }

        while (sum >> 16)
        {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        checksum = ~sum;
        return checksum;
    }
    else
    {
        return 0xFFFFFFFF;
    }
}


void OutputCurrentDirectory()
{
    char path[MAX_PATH];
    _getcwd(path, MAX_PATH);
    fprintf(stdout, "Current Directory: %s\n", path);
}