#pragma once

#define ETHERTYPE_VLAN 0x8100
#define ETHER_TYPE_DVLAN 0x88A8

struct eth_header {
    unsigned char destination[6];
    unsigned char source[6];
    uint16_t ethtype;
};

// structs
struct _ip_header
{
    uint8_t IHL : 4;
    uint8_t Version : 4;
    uint8_t TypeOfService;
    uint16_t TotalLength;
    uint16_t Identification;
    uint16_t Fragmentation;
    uint8_t TTL;
    uint8_t Protocol;
    uint16_t CheckSum;
    uint32_t SourceAddress;
    uint32_t DestinationAddress;
};

struct _icmp_header
{
    uint8_t type;
    uint8_t Code;
    uint16_t CheckSum;
    uint16_t icmp_id;
    uint16_t icmp_sequence;
};