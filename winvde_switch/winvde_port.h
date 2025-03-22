#pragma once

#define P_GETFLAG 0
#define P_SETFLAG 1
#define P_ADDFLAG 2
#define P_CLRFLAG 3

#define HUB_TAG 0x1

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
int close_ep_port_fd(uint32_t portno, int fd_ctl);
void vlanprintactive(int vlan, FILE* fd);
char* port_getuser(int uid);
char* port_getgroup(int gid);
int rec_close_ep(struct endpoint** pep, int fd_ctl);
int ep_get_port(struct endpoint* ep);