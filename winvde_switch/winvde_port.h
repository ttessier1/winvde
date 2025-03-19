#pragma once

#define P_GETFLAG 0
#define P_SETFLAG 1
#define P_ADDFLAG 2
#define P_CLRFLAG 3

#define HUB_TAG 0x1

void port_init(int init_num_ports);
int port_showinfo(FILE* fd);
int portsetnumports(int val);
int portsethub(int val);
int portsetvlan(char* arg);
int portcreateauto(FILE* fd);
int portcreate(int val);
int portremove(int val);
int portallocatable(char* arg);
int portsetuser(char* arg);
int portsetgroup(char* arg);
int epclose(char* arg);
int print_ptable(FILE* fd, char* arg);
int print_ptableall(FILE* fd, char* arg);
int vlancreate(int vlan);
int vlanremove(int vlan);
int vlanaddport(char* arg);
int vlandelport(char* arg);
int vlanprint(FILE* fd, char* arg);
int vlanprintall(FILE* fd, char* arg);
void vlanprintelem(int vlan, FILE* fd);
int vlancreate_nocheck(int vlan);
int portflag(int op, int f);
int alloc_port(unsigned int portno);
int print_port(FILE* fd, int i, int inclinactive);
void free_port(unsigned int portno);
int close_ep_port_fd(int portno, int fd_ctl);
void vlanprintactive(int vlan, FILE* fd);
char* port_getuser(int uid);
char* port_getgroup(int gid);
int rec_close_ep(struct endpoint** pep, int fd_ctl);
