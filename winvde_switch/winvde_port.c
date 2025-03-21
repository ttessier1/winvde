#include <winmeta.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


#include "winvde_port.h"
#include "winvde_switch.h"
#include "winvde_comlist.h"
#include "winvde_modsupport.h"
#include "winvde_hash.h"
#include "winvde_output.h"
#include "bitarray.h"
#include "winvde_loglevel.h"

#ifdef DEBUGOPT
#define DBGPORTNEW (dl) 
#define DBGPORTDEL (dl+1) 
#define DBGPORTDESCR (dl+2) 
#define DBGEPNEW (dl+3) 
#define DBGEPDEL (dl+4) 
#define PKTFILTIN (dl+5)
#define PKTFILTOUT (dl+6)
static struct dbgcl dl[] = {
	  {"port/+","new port",D_PORT | D_PLUS},
		{"port/-","closed port",D_PORT | D_MINUS},
		{"port/descr","set port description",D_PORT | D_DESCR},
	  {"port/ep/+","new endpoint",D_EP | D_PLUS},
		{"port/ep/-","closed endpoint",D_EP | D_MINUS},
		{"packet/in",NULL,D_PACKET | D_IN},
		{"packet/out",NULL,D_PACKET | D_OUT},
};
#endif


#define NOTINPOOL 0x8000

int pflag = 0;
int numports;
#ifdef VDE_PQ2
int stdqlen = 128;
#endif

struct endpoint {
	int port;
	int fd_ctl;
	int fd_data;
	char* descr;
#ifdef VDE_PQ2
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
	int (*sender)(int fd_ctl, int fd_data, void* packet, int len, int port);
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

#define STRSTATUS(PN,V) \
	((ba_check(vlant[(V)].notlearning,(PN))) ? "Discarding" : \
	 (ba_check(vlant[(V)].bctag,(PN)) || ba_check(vlant[(V)].bcuntag,(PN))) ? \
	 "Forwarding" : "Learning")



struct {
	bitarray table;
	bitarray bctag;
	bitarray bcuntag;
	bitarray notlearning;
} vlant[NUMOFVLAN + 1];
bitarray validvlan;


struct port** portv;
/*
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
*/
static struct comlist cl[] = {
	{"port","============","PORT STATUS MENU",NULL,NOARG},
	{"port/showinfo","","show port info",port_showinfo,NOARG | WITHFILE},
	{"port/setnumports","N","set the number of ports",portsetnumports,INTARG},
	/*{"port/setmacaddr","MAC","set the switch MAC address",setmacaddr,STRARG},*/
	{"port/sethub","0/1","1=HUB 0=switch",portsethub,INTARG},
	{"port/setvlan","N VLAN","set port VLAN (untagged)",portsetvlan,STRARG},
	{"port/createauto","","create a port with an automatically allocated id (inactive|notallocatable)",portcreateauto,NOARG | WITHFILE},
	{"port/create","N","create the port N (inactive|notallocatable)",portcreate,INTARG},
	{"port/remove","N","remove the port N",portremove,INTARG},
	{"port/allocatable","N 0/1","Is the port allocatable as unnamed? 1=Y 0=N",portallocatable,STRARG},
	{"port/setuser","N user","access control: set user",portsetuser,STRARG},
	{"port/setgroup","N user","access control: set group",portsetgroup,STRARG},
	{"port/epclose","N ID","remove the endpoint port N/id ID",epclose,STRARG},
#ifdef VDE_PQ2
	{"port/defqlen","LEN","set the default queue length for new ports",defqlen,INTARG},
	{"port/epqlen","N ID LEN","set the lenth of the queue for port N/id IP",epqlen,STRARG},
#endif
#ifdef PORTCOUNTERS
	{"port/resetcounter","[N]","reset the port (N) counters",portresetcounters,STRARG},
#endif
	{"port/print","[N]","print the port/endpoint table",print_ptable,STRARG | WITHFILE},
	{"port/allprint","[N]","print the port/endpoint table (including inactive port)",print_ptableall,STRARG | WITHFILE},
	{"vlan","============","VLAN MANAGEMENT MENU",NULL,NOARG},
	{"vlan/create","N","create the VLAN with tag N",vlancreate,INTARG},
	{"vlan/remove","N","remove the VLAN with tag N",vlanremove,INTARG},
	{"vlan/addport","N PORT","add port to the vlan N (tagged)",vlanaddport,STRARG},
	{"vlan/delport","N PORT","delete port from the vlan N (tagged)",vlandelport,STRARG},
	{"vlan/print","[N]","print the list of defined vlan",vlanprint,STRARG | WITHFILE},
	{"vlan/allprint","[N]","print the list of defined vlan (including inactive port)",vlanprintall,STRARG | WITHFILE},
};

void port_init(int init_num_ports)
{
	if ((numports = init_num_ports) <= 0)
	{
		printlog(LOG_ERR, "The switch must have at least 1 port\n");
		exit(1);
	}
	portv = calloc(numports, sizeof(struct port*));
	/* vlan_init */
	validvlan = bac_alloc(NUMOFVLAN);
	if (portv == NULL || validvlan == NULL) {
		printlog(LOG_ERR, "ALLOC port data structures");
		exit(1);
	}
	ADDCL(cl);
#ifdef DEBUGOPT
	ADDDBGCL(dl);
#endif
	if (vlancreate_nocheck(0) != 0) {
		printlog(LOG_ERR, "ALLOC vlan port data structures");
		exit(1);
	}
}

int port_showinfo(FILE* fd)
{
	printoutc(fd, "Numports=%d", numports);
	printoutc(fd, "HUB=%s", (pflag & HUB_TAG) ? "true" : "false");
#ifdef PORTCOUNTERS
	printoutc(fd, "counters=true");
#else
	printoutc(fd, "counters=false");
#endif
#ifdef VDE_PQ2
	printoutc(fd, "default length of port packet queues: %d", stdqlen);
#endif
	return 0;
}

int portsetnumports(int val)
{
	if (val > 0) {
		/*resize structs*/
		int index;
		for (index = val; index < numports; index++)
			if (portv[index] != NULL)
				return EADDRINUSE;
		portv = realloc(portv, val * sizeof(struct port*));
		if (portv == NULL) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "Numport resize failed portv %s", errorbuff);
			exit(1);
		}
		for (index = 0; index < NUMOFVLAN; index++) {
			if (vlant[index].table) {
				vlant[index].table = ba_realloc(vlant[index].table, numports, val);
				if (vlant[index].table == NULL) {
					strerror_s(errorbuff, sizeof(errorbuff), errno);
					printlog(LOG_ERR, "Numport resize failed vlan tables vlan table %s", errorbuff);
					exit(1);
				}
			}
			if (vlant[index].bctag) {
				vlant[index].bctag = ba_realloc(vlant[index].bctag, numports, val);
				if (vlant[index].bctag == NULL) {
					strerror_s(errorbuff, sizeof(errorbuff), errno);
					printlog(LOG_ERR, "Numport resize failed vlan tables vlan bctag %s", errorbuff);
					exit(1);
				}
			}
			if (vlant[index].bcuntag) {
				vlant[index].bcuntag = ba_realloc(vlant[index].bcuntag, numports, val);
				if (vlant[index].bcuntag == NULL) {
					strerror_s(errorbuff, sizeof(errorbuff), errno);
					printlog(LOG_ERR, "Numport resize failed vlan tables vlan bctag %s", errorbuff);
					exit(1);
				}
			}
			if (vlant[index].notlearning) {
				vlant[index].notlearning = ba_realloc(vlant[index].notlearning, numports, val);
				if (vlant[index].notlearning == NULL) {
					strerror_s(errorbuff, sizeof(errorbuff), errno);
					printlog(LOG_ERR, "Numport resize failed vlan tables vlan notlearning %s", errorbuff);
					exit(1);
				}
			}
		}
		for (index = numports; index < val; index++)
			portv[index] = NULL;
#ifdef FSTP
		fstsetnumports(val);
#endif
		numports = val;
		return 0;
	}
	else
		return EINVAL;
}

int portsethub(int val)
{
	if (val) {
#ifdef FSTP
		fstpshutdown();
#endif
		portflag(P_SETFLAG, HUB_TAG);
	}
	else
		portflag(P_CLRFLAG, HUB_TAG);
	return 0;
}

int portsetvlan(char* arg)
{
	int port, vlan;
	if (sscanf_s(arg, "%i %i", &port, &vlan) != 2)
		return EINVAL;
	/* port NOVLAN is okay here, it means NO untagged traffic */
	if (vlan <0 || vlan > NUMOFVLAN || port < 0 || port >= numports)
		return EINVAL;
	if ((vlan != NOVLAN && !bac_check(validvlan, vlan)) || portv[port] == NULL)
		return ENXIO;
	int oldvlan = portv[port]->vlanuntag;
	portv[port]->vlanuntag = NOVLAN;
	hash_delete_port(port);
	if (portv[port]->ep != NULL) {
		/*changing active port*/
		if (oldvlan != NOVLAN)
			ba_clr(vlant[oldvlan].bcuntag, port);
		if (vlan != NOVLAN) {
			ba_set(vlant[vlan].bcuntag, port);
			ba_clr(vlant[vlan].bctag, port);
		}
#ifdef FSTP
		if (oldvlan != NOVLAN) fstdelport(oldvlan, port);
		if (vlan != NOVLAN) fstaddport(vlan, port, 0);
#endif
	}
	if (oldvlan != NOVLAN) ba_clr(vlant[oldvlan].table, port);
	if (vlan != NOVLAN) ba_set(vlant[vlan].table, port);
	portv[port]->vlanuntag = vlan;
	return 0;
}

int portcreateauto(FILE* fd)
{
	int port = alloc_port(0);

	if (port < 0)
		return ENOSPC;

	portv[port]->flag |= NOTINPOOL;
	printoutc(fd, "Port %04d", port);
	return 0;
}

int portcreate(int val)
{
	int port;
	if (val < 0 || val >= numports)
		return EINVAL;
	if (portv[val] != NULL)
		return EEXIST;
	port = alloc_port(val);
	if (port < 0)
		return ENOSPC;
	portv[port]->flag |= NOTINPOOL;
	return 0;
}

int portremove(int val)
{
	if (val < 0 || val >= numports)
		return EINVAL;
	if (portv[val] == NULL)
		return ENXIO;
	if (portv[val]->ep != NULL)
		return EADDRINUSE;
	free_port(val);
	return 0;
}

int portallocatable(char* arg)
{
	int port, value;
	if (sscanf_s(arg, "%i %i", &port, &value) != 2)
		return EINVAL;
	if (port < 0 || port >= numports)
		return EINVAL;
	if (portv[port] == NULL)
		return ENXIO;
	if (value)
		portv[port]->flag &= ~NOTINPOOL;
	else
		portv[port]->flag |= NOTINPOOL;
	return 0;
}

int portsetuser(char* arg)
{
	int port;
	char* portuid = arg;
	while (*portuid != 0 && *portuid == ' ') portuid++;
	while (*portuid != 0 && *portuid != ' ') portuid++;
	while (*portuid != 0 && *portuid == ' ') portuid++;
	if (sscanf_s(arg, "%i", &port) != 1 || *portuid == 0)
		return EINVAL;
	if (port < 0 || port >= numports)
		return EINVAL;
	if (portv[port] == NULL)
		return ENXIO;
	//if ((pw = getpwnam(portuid)) != NULL)
	//	portv[port]->user = pw->pw_uid;
	//if (isdigit(*portuid))
	//	portv[port]->user = atoi(portuid);
	//else if (strcmp(portuid, "NONE") == 0 || strcmp(portuid, "ANY") == 0)
		portv[port]->user = -1;
	//else
	//	return EINVAL;
	return 0;
}

int portsetgroup(char* arg)
{
	int port;
	char* portgid = arg;
	while (*portgid != 0 && *portgid == ' ') portgid++;
	while (*portgid != 0 && *portgid != ' ') portgid++;
	while (*portgid != 0 && *portgid == ' ') portgid++;
	if (sscanf_s(arg, "%i", &port) != 1 || *portgid == 0)
		return EINVAL;
	if (port < 0 || port >= numports)
		return EINVAL;
	if (portv[port] == NULL)
		return ENXIO;
	//if ((gr = getgrnam(portgid)) != NULL)
	//	portv[port]->group = gr->gr_gid;
	//else if (isdigit(*portgid))
	//	portv[port]->group = atoi(portgid);
	//else if (strcmp(portgid, "NONE") == 0 || strcmp(portgid, "ANY") == 0)
		portv[port]->group = -1;
	//else
	//	return EINVAL;
	return 0;
}

int epclose(char* arg)
{
	int port, id;
	if (sscanf_s(arg, "%i %i", &port, &id) != 2)
	{
		return EINVAL;
	}
	else
	{
		return close_ep_port_fd(port, id);
	}
}

int print_ptable(FILE* fd, char* arg)
{
	int index;
	if (*arg != 0) {
		index = atoi(arg);
		if (index < 0 || index >= numports)
			return EINVAL;
		else {
			return print_port(fd, index, 0);
		}
	}
	else {
		for (index = 0; index < numports; index++)
			print_port(fd, index, 0);
		return 0;
	}
}

int print_ptableall(FILE* fd, char* arg)
{
	int index;
	if (*arg != 0) {
		index = atoi(arg);
		if (index < 0 || index >= numports)
			return EINVAL;
		else {
			return print_port(fd, index, 1);
		}
	}
	else {
		for (index = 0; index < numports; index++)
			print_port(fd, index, 1);
		return 0;
	}
}

int vlancreate(int vlan)
{
	if (vlan > 0 && vlan < NUMOFVLAN - 1) { /*vlan NOVLAN (0xfff a.k.a. 4095) is reserved */
		if (bac_check(validvlan, vlan))
			return EEXIST;
		else
			return vlancreate_nocheck(vlan);
	}
	else
		return EINVAL;
}

int vlanremove(int vlan)
{
	int index = 0;
	int is_used = 0;
	if (vlan >= 0 && vlan < NUMOFVLAN)
	{
		if (bac_check(validvlan, vlan))
		{
			
			ba_FORALL(1,vlant[vlan].table, numports, is_used++, index);
			if (is_used)
			{
				return EADDRINUSE;
			}
			else
			{
				bac_clr(validvlan, NUMOFVLAN, vlan);
				free(vlant[vlan].table);
				free(vlant[vlan].bctag);
				free(vlant[vlan].bcuntag);
				free(vlant[vlan].notlearning);
				vlant[vlan].table = NULL;
				vlant[vlan].bctag = NULL;
				vlant[vlan].bcuntag = NULL;
				vlant[vlan].notlearning = NULL;
#ifdef FSTP
				fstremovevlan(vlan);
#endif
				return 0;
			}
		}
		else
		{
			return ENXIO;
		}
	}
	else
	{
		return EINVAL;
	}
}

int vlanaddport(char* arg)
{
	int port, vlan;
	if (sscanf_s(arg, "%i %i", &vlan, &port) != 2)
		return EINVAL;
	if (vlan < 0 || vlan >= NUMOFVLAN - 1 || port < 0 || port >= numports)
		return EINVAL;
	if (!bac_check(validvlan, vlan) || portv[port] == NULL)
		return ENXIO;
	if (portv[port]->ep != NULL && portv[port]->vlanuntag != vlan) {
		/* changing active port*/
		ba_set(vlant[vlan].bctag, port);
#ifdef FSTP
		fstaddport(vlan, port, 1);
#endif
	}
	ba_set(vlant[vlan].table, port);
	return 0;
}

int vlandelport(char* arg)
{
	int port, vlan;
	if (sscanf_s(arg, "%i %i", &vlan, &port) != 2)
		return EINVAL;
	if (vlan < 0 || vlan >= NUMOFVLAN - 1 || port < 0 || port >= numports)
		return EINVAL;
	if (!bac_check(validvlan, vlan) || portv[port] == NULL)
		return ENXIO;
	if (portv[port]->vlanuntag == vlan)
		return EADDRINUSE;
	if (portv[port]->ep != NULL) {
		/*changing active port*/
		ba_clr(vlant[vlan].bctag, port);
#ifdef FSTP
		fstdelport(vlan, port);
#endif
	}
	ba_clr(vlant[vlan].table, port);
	hash_delete_port(port);
	return 0;
}


int vlanprint(FILE* fd, char* arg)
{
	if (*arg != 0) {
		int vlan;
		vlan = atoi(arg);
		if (vlan >= 0 && vlan < NUMOFVLAN - 1) {
			if (bac_check(validvlan, vlan))
				vlanprintactive(vlan, fd);
			else
				return ENXIO;
		}
		else
			return EINVAL;
	}
	else
		bac_FORALLFUN(1, validvlan, NUMOFVLAN, vlanprintactive, fd);
	return 0;
}

int vlanprintall(FILE* fd, char* arg)
{
	if (*arg != 0) {
		int vlan;
		vlan = atoi(arg);
		if (vlan > 0 && vlan < NUMOFVLAN - 1) {
			if (bac_check(validvlan, vlan))
				vlanprintelem(vlan, fd);
			else
				return ENXIO;
		}
		else
			return EINVAL;
	}
	else
		bac_FORALLFUN(1,validvlan, NUMOFVLAN, vlanprintelem, fd);
	return 0;
}

void vlanprintelem(int vlan, FILE* fd)
{
	uint32_t index;
	printoutc(fd, "VLAN %04d", vlan);
	ba_FORALL(1,vlant[vlan].table, (uint32_t)numports,
		printoutc(fd, " -- Port %04d tagged=%d active=%d status=%s",
			index, portv[index]->vlanuntag != vlan, portv[index]->ep != NULL, STRSTATUS(index, vlan)), index);
}

int vlancreate_nocheck(int vlan)
{
	int rv = 0;
	vlant[vlan].table = ba_alloc(numports);
	vlant[vlan].bctag = ba_alloc(numports);
	vlant[vlan].bcuntag = ba_alloc(numports);
	vlant[vlan].notlearning = ba_alloc(numports);
	if (vlant[vlan].table == NULL || vlant[vlan].bctag == NULL ||
		vlant[vlan].bcuntag == NULL)
		return ENOMEM;
	else {
#ifdef FSTP
		rv = fstnewvlan(vlan);
#endif
		if (rv == 0) {
			bac_set(validvlan, NUMOFVLAN, vlan);
		}
		return rv;
	}
}

int portflag(int op, int f)
{
	int oldflag = pflag;
	switch (op) {
	case P_GETFLAG: oldflag = pflag & f; break;
	case P_SETFLAG: pflag = f; break;
	case P_ADDFLAG: pflag |= f; break;
	case P_CLRFLAG: pflag &= ~f; break;
	}
	return oldflag;
}

int alloc_port(unsigned int portno)
{
	int index = portno;
	if (index == 0) {
		/* take one */
		for (index = 1; index < numports && portv[index] != NULL &&
			(portv[index]->ep != NULL || portv[index]->flag & NOTINPOOL); index++)
			;
	}
	else if (index < 0) /* special case MGMT client port */
		index = 0;
	if (index >= numports)
		return -1;
	else {
		if (portv[index] == NULL) {
			struct port* port;
			if ((port = malloc(sizeof(struct port))) == NULL) {
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printlog(LOG_WARNING, "malloc port %s", errorbuff);
				return -1;
			}
			else
			{
#ifdef DEBUGOPT
				DBGOUT(DBGPORTNEW, "%02d", i);
				EVENTOUT(DBGPORTNEW, i);
#endif
				portv[index] = port;
				port->ep = NULL;
				port->user = port->group = port->curuser = -1;
#ifdef FSTP
				port->cost = DEFAULT_COST;
#endif
#ifdef PORTCOUNTERS
				port->pktsin = 0;
				port->pktsout = 0;
				port->bytesin = 0;
				port->bytesout = 0;
#endif
				port->flag = 0;
				port->sender = NULL;
				port->vlanuntag = 0;
				ba_set(vlant[0].table, index);
			}
		}
		return index;
	}
}

int print_port(FILE* fd, int index, int inclinactive)
{
	struct endpoint* ep;
	if (portv[index] != NULL && (inclinactive || portv[index]->ep != NULL)) {
		printoutc(fd, "Port %04d untagged_vlan=%04d %sACTIVE - %sUnnamed Allocatable",
			index, portv[index]->vlanuntag,
			portv[index]->ep ? "" : "IN",
			(portv[index]->flag & NOTINPOOL) ? "NOT " : "");
		printoutc(fd, " Current User: %s Access Control: (User: %s - Group: %s)",
			port_getuser(portv[index]->curuser),
			port_getuser(portv[index]->user),
			port_getgroup(portv[index]->group));
#ifdef PORTCOUNTERS
		printoutc(fd, " IN:  pkts %10lld          bytes %20lld", portv[i]->pktsin, portv[i]->bytesin);
		printoutc(fd, " OUT: pkts %10lld          bytes %20lld", portv[i]->pktsout, portv[i]->bytesout);
#endif
		for (ep = portv[index]->ep; ep != NULL; ep = ep->next) {
			printoutc(fd, "  -- endpoint ID %04d module %-12s: %s", ep->fd_ctl,
				portv[index]->ms->modname, (ep->descr) ? ep->descr : "no endpoint description");
#ifdef VDE_PQ2
			printoutc(fd, "              unsent packets: %d max %d", ep->vdepq_count, ep->vdepq_max);
#endif
		}
		return 0;
	}
	else
		return ENXIO;
}

void free_port(unsigned int portno)
{
	if (portno < (uint32_t)numports) {
		struct port* port = portv[portno];
		if (port != NULL && port->ep == NULL) {
			portv[portno] = NULL;
			int index;
			/* delete completely the port. all vlan defs zapped */
			bac_FORALL(1,validvlan, NUMOFVLAN, ba_clr(vlant[index].table, portno), index);
			free(port);
		}
	}
}

int close_ep_port_fd(int portno, int fd_ctl)
{
	int rv = 0;
	struct port* port = NULL;
	if (portno >= 0 && portno < numports) {
		port = portv[portno];
		if (port != NULL) {
			rv = rec_close_ep(&(port->ep), fd_ctl);
			if (port->ep == NULL) {
#ifdef DEBUGOPT
				DBGOUT(DBGPORTDEL, "%02d", portno);
				EVENTOUT(DBGPORTDEL, portno);
#endif
				hash_delete_port(portno);
				port->ms = NULL;
				port->sender = NULL;
				port->curuser = -1;
				int index;
				/* inactivate port: all active vlan defs cleared 
				bac_FORALL(validvlan, NUMOFVLAN, ({
							ba_clr(vlant[i].bctag,portno);
#ifdef FSTP
							fstdelport(i,portno);
#endif
					}), i);*/
#ifdef FSTP
				bac_FORALL(validvlan, NUMOFVLAN, ({
							ba_clr(vlant[i].bctag,portno);
							fstdelport(i,portno);
					}), i);
#else
				bac_FORALL(1,validvlan, NUMOFVLAN,ba_clr(vlant[index].bctag,portno), index);
#endif


				if (port->vlanuntag < NOVLAN) ba_clr(vlant[port->vlanuntag].bcuntag, portno);
			}
			return rv;
		}
		else
			return ENXIO;
	}
	else
	{
		return EINVAL;
	}
}

void vlanprintactive(int vlan, FILE* fd)
{
	int index;
	printoutc(fd, "VLAN %04d", vlan);
#ifdef FSTP
	if (pflag & FSTP_TAG) {
#if 0
		printoutc(fd, " ++ FST root %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x \n"
			"        designated %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x port %d cost %d age %d",
			fsttab[vlan]->root[0], fsttab[vlan]->root[1], fsttab[vlan]->root[2], fsttab[vlan]->root[3],
			fsttab[vlan]->root[4], fsttab[vlan]->root[5], fsttab[vlan]->root[6], fsttab[vlan]->root[7],
			fsttab[vlan]->desbr[0], fsttab[vlan]->desbr[1], fsttab[vlan]->desbr[2], fsttab[vlan]->desbr[3],
			fsttab[vlan]->desbr[4], fsttab[vlan]->desbr[5], fsttab[vlan]->desbr[6], fsttab[vlan]->desbr[7],
			fsttab[vlan]->rootport,
			ntohl(*(u_int32_t*)(&(fsttab[vlan]->rootcost))),
			qtime() - fsttab[vlan]->roottimestamp);
		ba_FORALL(vlant[vlan].table, numports,
			({ int tagged = portv[i]->vlanuntag != vlan;
			 if (portv[i]->ep)
			 printoutc(fd," -- Port %04d tagged=%d act=%d learn=%d forw=%d cost=%d role=%s",
				 i, tagged, 1, !(NOTLEARNING(i,vlan)),
				 (tagged) ? (ba_check(vlant[vlan].bctag,i) != 0) : (ba_check(vlant[vlan].bcuntag,i) != 0),
				 portv[i]->cost,
				 (fsttab[vlan]->rootport == i ? "Root" :
					((ba_check(fsttab[vlan]->backup,i) ? "Alternate/Backup" : "Designated")))
				 ); 0;
				}), i);
#endif
	}
	else {
#endif

		unsigned int __i, __j; \
			bitarrayelem __v; \
			unsigned int max = __WORDSIZEROUND(numports);
			for (__i = 0; __i < max; __i++)
			{
				for (__v = (vlant[vlan].table)[__i], __j = 0; __j < __VDEWORDSIZE; __v >>= 1, __j++)
				{
					if (__v & 1)
					{
						(index) = __i * __VDEWORDSIZE + __j;
						int tagged = portv[index]->vlanuntag != vlan; 
						if (portv[index]->ep)
						{
							printoutc(fd, " -- Port %04d tagged=%d active=1 status=%s", index, tagged, STRSTATUS(index, vlan));
						}
					}
				}
			}
			//X = vlant[vlan].table;
			//N = numports
			//EXPR = ({ int tagged = portv[i]->vlanuntag != vlan;if (portv[i]->ep) printoutc(fd, " -- Port %04d tagged=%d active=1 status=%s", i, tagged,STRSTATUS(i, vlan));})
			// K = i
		/*ba_FORALL(vlant[vlan].table, numports,
			({ int tagged = portv[i]->vlanuntag != vlan;
			 if (portv[i]->ep)
			 printoutc(fd," -- Port %04d tagged=%d active=1 status=%s", i, tagged,
				 STRSTATUS(i,vlan));
				}), i);*/
#ifdef FSTP
	}
#endif
}

char* port_getuser(int uid)
{
	static char buf[20];
	if (uid == -1)
		return "NONE";
	else {
		_itoa_s(uid, buf, sizeof(buf), 10);
		return &buf[0];
	}
}

char* port_getgroup(int gid)
{
	static char buf[20];
	if (gid == -1)
	{
		return "NONE";
	}
	else 
	{
		_itoa_s(gid,buf,sizeof(buf), 10);
		return &buf[0];
	}
}


int rec_close_ep(struct endpoint** pep, int fd_ctl)
{
	struct endpoint* this = *pep;
	if (this != NULL) {
		if (this->fd_ctl == fd_ctl) {
#ifdef DEBUGOPT
			DBGOUT(DBGEPDEL, "Port %02d FD %2d", this->port, fd_ctl);
			EVENTOUT(DBGEPDEL, this->port, fd_ctl);
#endif
			*pep = this->next;
#ifdef VDE_PQ2
			vdepq_del(&(this->vdepq));
#endif
			if (portv[this->port]->ms->delep)
				portv[this->port]->ms->delep(this->fd_ctl, this->fd_data, this->descr);
			free(this);
			return 0;
		}
		else
			return rec_close_ep(&(this->next), fd_ctl);
	}
	else
		return ENXIO;
}

int ep_get_port(struct endpoint* ep)
{
	return ep->port;
}
