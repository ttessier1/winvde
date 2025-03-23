#pragma once
#include <WinSock2.h>
#include "winvde_switch.h"

#define P_GETFLAG 0
#define P_SETFLAG 1
#define P_ADDFLAG 2
#define P_CLRFLAG 3

#define HUB_TAG 0x1

#define ETH_HEADER_SIZE 14
/* a full ethernet 802.3 frame */
#include <pshpack1.h>
struct ethheader {
	unsigned char dest[ETH_ALEN];
	unsigned char src[ETH_ALEN];
	unsigned char proto[2];
};

struct packet {
	struct ethheader header;
	unsigned char data[1504]; /*including trailer, IF ANY */
};

struct bipacket {
	char filler[4];
	struct packet p;
};

struct endpoint {
	int port;
	SOCKET fd_ctl;
	SOCKET fd_data;
	char* descr;
#if defined(VDE_PQ2)
	struct vdepq* vdepq;
	int vdepq_count;
	int vdepq_max;
#endif
	struct endpoint* next;
};

struct port {
	struct endpoint* ep;
	int flag;
	/* sender is already inside ms, but it needs one more memaccess */
	int (*sender)(SOCKET fd_ctl, SOCKET fd_data, void* packet, int len, int port);
	struct mod_support* ms;
	int vlanuntag;
	uint32_t user;
	uint32_t group;
	uint32_t curuser;
#ifdef FSTP
	int cost;
#endif
#ifdef PORTCOUNTERS
	long long pktsin, pktsout, bytesin, bytesout;
#endif
};

#include <poppack.h>

void port_init(int init_num_ports);
int port_showinfo(struct comparameter* parameter);
int portsetnumports(struct comparameter* parameter);
int portsethub(struct comparameter* parameter);
int portsetvlan(struct comparameter* parameter);
int portcreateauto(struct comparameter* parameter);
int portcreate(struct comparameter* parameter);
int portremove(struct comparameter* parameter);
int portallocatable(struct comparameter* parameter);
int portsetuser(struct comparameter* parameter);
int portsetgroup(struct comparameter* parameter);
int epclose(struct comparameter* parameter);
int print_ptable(struct comparameter * parameter);
int print_ptableall(struct comparameter* parameter);
int vlancreate(struct comparameter* parameter);
int vlanremove(struct comparameter* parameter);
int vlanaddport(struct comparameter* parameter);
int vlandelport(struct comparameter* parameter);
int vlanprint(struct comparameter* parameter);
int vlanprintall(struct comparameter* parameter);
void vlanprintelem(int vlan, FILE* fd);
int vlancreate_nocheck(int vlan);
int portflag(int op, int f);
int alloc_port(unsigned int portno);
int print_port(FILE* fd, int index, int inclinactive);
void free_port(unsigned int portno);
int close_ep_port_fd(uint32_t portno, SOCKET fd_ctl);
void vlanprintactive(int vlan, FILE* fd);
char* port_getuser(int uid);
char* port_getgroup(int gid);
int rec_close_ep(struct endpoint** pep, SOCKET fd_ctl);
int close_ep(struct endpoint* ep);
int ep_get_port(struct endpoint* ep);
void handle_in_packet(struct endpoint* ep, struct packet* packet, int len);
void setup_description(struct endpoint* ep, char* descr);
struct endpoint* setup_ep(int portno, SOCKET fd_ctl, SOCKET fd_data, uint32_t user, struct mod_support* modfun);
int checkport_ac(struct port* port, uint32_t user);