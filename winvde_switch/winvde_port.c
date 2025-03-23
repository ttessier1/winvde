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
#include "winvde_debugcl.h"
#include "winvde_debugopt.h"
#include "winvde_event.h"
#include "winvde_user.h"


#define D_PACKET 01000
#define D_MGMT 02000
#define D_IN 01
#define D_OUT 02
#define D_PLUS 01
#define D_MINUS 02
#define D_DESCR 03
#define D_STATUS 04
#define D_ROOT 05
#define D_HASH 010
#define D_PORT 020
#define D_EP 030
#define D_FSTP 040

#if defined(DEBUGOPT)
#define DBGPORTNEW (dl) 
#define DBGPORTDEL (dl+1) 
#define DBGPORTDESCR (dl+2) 
#define DBGEPNEW (dl+3) 
#define DBGEPDEL (dl+4) 
#define PKTFILTIN (dl+5)
#define PKTFILTOUT (dl+6)
struct dbgcl dl[] = {
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
uint32_t numports;
#ifdef VDE_PQ2
int stdqlen = 128;
#endif



#define TAG2UNTAG(P,LEN) \
	memmove((char *)(P)+4,(P),2*ETH_ALEN); LEN -= 4 ; \
	(struct packet *)((char *)(P)+4); 

#define UNTAG2TAG(P,VLAN,LEN) \
	memmove((char *)(P)-4,(P),2*ETH_ALEN); LEN += 4 ; \
	(P)->header.src[2]=0x81; (P)->header.src[3]=0x00;\
	(P)->header.src[4]=(VLAN >> 8); (P)->header.src[5]=(VLAN);\
	(struct packet *)((char *)(P)-4); 

#define STRSTATUS(PN,V) \
	((ba_check(vlant[(V)].notlearning,(PN))) ? "Discarding" : \
	 (ba_check(vlant[(V)].bctag,(PN)) || ba_check(vlant[(V)].bcuntag,(PN))) ? \
	 "Forwarding" : "Learning")

#if !defined(VDE_PQ2)
#define SEND_PACKET_PORT(PORT,PORTNO,PACKET,LEN) \
	 struct port *Port=(PORT); \
	 if (packetfilter(PKTFILTOUT,(PORTNO),(PACKET), (LEN))) {\
	 struct endpoint *ep; \
	 SEND_COUNTER_UPD(Port,LEN); \
	 for (ep=Port->ep; ep != NULL; ep=ep->next) \
	 Port->ms->sender(ep->fd_ctl, ep->fd_data, (PACKET), (LEN), ep->port); \
	 }
#else
#define SEND_PACKET_PORT(PORT,PORTNO,PACKET,LEN,TMPBUF) \
	 struct port *Port=(PORT); \
	 if (packetfilter(PKTFILTOUT,(PORTNO),(PACKET), (LEN))) {\
	 struct endpoint *ep; \
	 SEND_COUNTER_UPD(Port,LEN); \
	 for (ep=Port->ep; ep != NULL; ep=ep->next) \
	 if (ep->vdepq_count || \
		 Port->ms->sender(ep->fd_ctl, ep->fd_data, (PACKET), (LEN), ep->port) == -EWOULDBLOCK) {\
	 if (ep->vdepq_count < ep->vdepq_max) \
	 ep->vdepq_count += vdepq_add(&(ep->vdepq), (PACKET), (LEN), TMPBUF); \
	 if (ep->vdepq_count == 1) mainloop_pollmask_add(ep->fd_data, POLLOUT);\
	 } \
	 } 
#endif

#ifdef PORTCOUNTERS
#define SEND_COUNTER_UPD(Port,LEN)\
	Port->pktsout++; \
	Port->bytesout +=len;
#else
#define SEND_COUNTER_UPD(Port,LEN)
#endif

#define IS_BROADCAST(addr)\
	((addr[0] & 1) == 1)

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
struct comlist cl[] = {
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
#if defined(DEBUGOPT)
	adddbgcl(sizeof(dl) / sizeof(struct dbgcl), dl);
#endif
	if (vlancreate_nocheck(0) != 0) {
		printlog(LOG_ERR, "ALLOC vlan port data structures");
		exit(1);
	}
}

int port_showinfo(struct comparameter * parameter)
{
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if(parameter->type1 != com_type_file|| parameter->data1.file_descriptor==NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->data1.file_descriptor != NULL)
	{
		printoutc(parameter->data1.file_descriptor, "Numports=%d", numports);
		printoutc(parameter->data1.file_descriptor, "HUB=%s", (pflag & HUB_TAG) ? "true" : "false");
#ifdef PORTCOUNTERS
		printoutc(parameter->data1.file_descriptor, "counters=true");
#else
		printoutc(parameter->data1.file_descriptor, "counters=false");
#endif
#ifdef VDE_PQ2
		printoutc(parameter->data1.file_descriptor, "default length of port packet queues: %d", stdqlen);
#endif
	}
	return 0;
}

int portsetnumports(struct comparameter* parameter)
{
	uint32_t index = 0;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		if (parameter->paramValue.intValue > 0) {
			/*resize structs*/
			
			for (index = parameter->paramValue.intValue; index < numports; index++)
			{
				if (portv[index] != NULL)
				{
					return EADDRINUSE;
				}
			}
			portv = (struct port**)realloc(portv, parameter->paramValue.intValue * sizeof(struct port*));
			if (portv == NULL)
			{
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printlog(LOG_ERR, "Numport resize failed portv %s", errorbuff);
				exit(1);
			}
			for (index = 0; index < NUMOFVLAN; index++)
			{
				if (vlant[index].table)
				{
					vlant[index].table = ba_realloc(vlant[index].table, numports, parameter->paramValue.intValue);
					if (vlant[index].table == NULL)
					{
						strerror_s(errorbuff, sizeof(errorbuff), errno);
						printlog(LOG_ERR, "Numport resize failed vlan tables vlan table %s", errorbuff);
						exit(1);
					}
				}
				if (vlant[index].bctag)
				{
					vlant[index].bctag = ba_realloc(vlant[index].bctag, numports, parameter->paramValue.intValue);
					if (vlant[index].bctag == NULL)
					{
						strerror_s(errorbuff, sizeof(errorbuff), errno);
						printlog(LOG_ERR, "Numport resize failed vlan tables vlan bctag %s", errorbuff);
						exit(1);
					}
				}
				if (vlant[index].bcuntag)
				{
					vlant[index].bcuntag = ba_realloc(vlant[index].bcuntag, numports, parameter->paramValue.intValue);
					if (vlant[index].bcuntag == NULL)
					{
						strerror_s(errorbuff, sizeof(errorbuff), errno);
						printlog(LOG_ERR, "Numport resize failed vlan tables vlan bctag %s", errorbuff);
						exit(1);
					}
				}
				if (vlant[index].notlearning)
				{
					vlant[index].notlearning = ba_realloc(vlant[index].notlearning, numports, parameter->paramValue.intValue);
					if (vlant[index].notlearning == NULL)
					{
						strerror_s(errorbuff, sizeof(errorbuff), errno);
						printlog(LOG_ERR, "Numport resize failed vlan tables vlan notlearning %s", errorbuff);
						exit(1);
					}
				}
			}
			for (index = numports; index < parameter->paramValue.intValue; index++)
			{
				portv[index] = NULL;
			}
#ifdef FSTP
			fstsetnumports(val);
#endif
			numports = parameter->paramValue.intValue;
			return 0;
		}
		else
		{
			return EINVAL;
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

int portsethub(struct comparameter* parameter)
{
	if(!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		if (parameter->paramValue.intValue)
		{
#ifdef FSTP
			fstpshutdown();
#endif
			portflag(P_SETFLAG, HUB_TAG);
		}
		else
		{
			portflag(P_CLRFLAG, HUB_TAG);
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int portsetvlan(struct comparameter* parameter)
{
	uint32_t port = 0;
	int vlan = 0;
	int oldvlan = 0;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_string)
	{
		if (sscanf_s(parameter->paramValue.stringValue, "%i %i", &port, &vlan) != 2)
		{
			return EINVAL;
		}
		/* port NOVLAN is okay here, it means NO untagged traffic */
		if (vlan <0 || vlan > NUMOFVLAN || port < 0 || port >= numports)
		{
			return EINVAL;
		}
		if ((vlan != NOVLAN && !bac_check(validvlan, vlan)) || portv[port] == NULL)
		{
			return ENXIO;
		}
		oldvlan = portv[port]->vlanuntag;
		portv[port]->vlanuntag = NOVLAN;
		hash_delete_port(port);
		if (portv[port]->ep != NULL)
		{
			/*changing active port*/
			if (oldvlan != NOVLAN)
			{
				ba_clr(vlant[oldvlan].bcuntag, port);
			}
			if (vlan != NOVLAN)
			{
				ba_set(vlant[vlan].bcuntag, port);
				ba_clr(vlant[vlan].bctag, port);
			}
#ifdef FSTP
			if (oldvlan != NOVLAN)
			{
				fstdelport(oldvlan, port);
			}
			if (vlan != NOVLAN)
			{
				fstaddport(vlan, port, 0);
			}
#endif
		}
		if (oldvlan != NOVLAN)
		{
			ba_clr(vlant[oldvlan].table, port);
		}
		if (vlan != NOVLAN)
		{
			ba_set(vlant[vlan].table, port);
		}
		portv[port]->vlanuntag = vlan;
	}
	return 0;
}

int portcreateauto(struct comparameter* parameter/*FILE* fd*/)
{
	int port = 0; 
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->paramType == com_param_type_null )
	{
		port = alloc_port(0);
		if (port < 0)
		{
			return ENOSPC;
		}

		portv[port]->flag |= NOTINPOOL;
		printoutc(parameter->data1.file_descriptor, "Port %04d", port);
	}
	return 0;
}

int portcreate(struct comparameter* parameter)
{
	int port = 0 ;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (portv == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_unsigned_int)
	{
		if (parameter->paramValue.intValue < 0 || parameter->paramValue.intValue >= numports)
		{
			return EINVAL;
		}
		if (portv[parameter->paramValue.intValue] != NULL)
		{
			return EEXIST;
		}
		port = alloc_port(parameter->paramValue.intValue);
		if (port < 0)
		{
			return ENOSPC;
		}
		portv[port]->flag |= NOTINPOOL;
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int portremove(struct comparameter* parameter)
{
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (portv == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_unsigned_int)
	{
		if (parameter->paramValue.intValue < 0 || parameter->paramValue.intValue >= numports)
		{
			return EINVAL;
		}
		if (portv[parameter->paramValue.intValue] == NULL)
		{
			return ENXIO;
		}
		if (portv[parameter->paramValue.intValue]->ep != NULL)
		{
			return EADDRINUSE;
		}
		free_port(parameter->paramValue.intValue);
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int portallocatable(struct comparameter* parameter)
{
	uint32_t port= 0 ; 
	int value = 0 ;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL)
	{
		if (sscanf_s(parameter->paramValue.stringValue, "%i %i", &port, &value) != 2)
		{
			return EINVAL;
		}
		if (port < 0 || port >= numports)
		{
			return EINVAL;
		}
		if (portv[port] == NULL)
		{
			return ENXIO;
		}
		if (value)
		{
			portv[port]->flag &= ~NOTINPOOL;
		}
		else
		{
			portv[port]->flag |= NOTINPOOL;
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int portsetuser(struct comparameter* parameter)
{
	uint32_t port = 0 ;
	uint32_t user_id = 0;
	char* portuid =NULL;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL)
	{
		portuid = parameter->paramValue.stringValue;
		while (*portuid != 0 && *portuid == ' ')
		{
			portuid++;
		}
		while (*portuid != 0 && *portuid != ' ')
		{
			portuid++;
		}
		while (*portuid != 0 && *portuid == ' ')
		{
			portuid++;
		}
		if (sscanf_s(parameter->paramValue.stringValue, "%i", &port) != 1 || *portuid == 0)
		{
			return EINVAL;
		}
		if (port < 0 || port >= numports)
		{
			return EINVAL;
		}
		if (portv[port] == NULL)
		{
			return ENXIO;
		}
		user_id = GetUserIdFunction();
		if (user_id != -1)
		{
			portv[port]->user = user_id;
		}
		else if (strcmp(portuid, "NONE") == 0 || strcmp(portuid, "ANY") == 0)
		{
			portv[port]->user = -1;
		}
		else
		{
			return EINVAL;
		}
	}
	else
	{
		errno = EINVAL;
		return -1 ;
	}
	return 0;
}

int portsetgroup(struct comparameter* parameter)
{
	uint32_t port = 0 ;
	char* portgid = NULL;
	uint32_t group_id = 0;
	struct group* groupval=NULL;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL)
	{
		portgid = parameter->paramValue.stringValue;
		while (*portgid != 0 && *portgid == ' ')
		{
			portgid++;
		}
		while (*portgid != 0 && *portgid != ' ')
		{
			portgid++;
		}
		while (*portgid != 0 && *portgid == ' ')
		{
			portgid++;
		}
		if (sscanf_s(parameter->paramValue.stringValue, "%i", &port) != 1 || *portgid == 0)
		{
			return EINVAL;
		}
		if (port < 0 || port >= numports)
		{
			return EINVAL;
		}
		if (portv[port] == NULL)
		{
			return ENXIO;
		}
		groupval = GetGroupFunction(parameter->paramValue.stringValue);
		if (groupval != NULL)
		{
			portv[port]->group = groupval->groupid;
		}
		else if (strcmp(portgid, "NONE") == 0 || strcmp(portgid, "ANY") == 0)
		{
			portv[port]->group = -1;
		}
		else
		{
			return EINVAL;
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int epclose(struct comparameter* parameter)
{

	int port = 0 ;
	int id = 0 ;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL)
	{
		if (sscanf_s(parameter->paramValue.stringValue, "%i %i", &port, &id) != 2)
		{
			return EINVAL;
		}
		else
		{
			return close_ep_port_fd(port, id);
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

//int print_ptable(FILE* fd, char* arg)
int print_ptable(struct comparameter * parameter)
{
	uint32_t index = 0;
	if (!parameter) {
		errno = EINVAL;
		return - 1;
	}
	if (parameter->type1 != com_type_file || parameter->data1.file_descriptor == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if(parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL )
	{
		index = atoi(parameter->paramValue.stringValue);
		if (index < 0 || index >= numports)
		{
			return EINVAL;
		}
		else 
		{
			return print_port(parameter->data1.file_descriptor, index, 0);
		}
	}
	else
	{
		for (index = 0; index < numports; index++)
		{
			print_port(parameter->data1.file_descriptor, index, 0);
		}
		return 0;
	}
}

//int print_ptableall(FILE* fd, char* arg)
int print_ptableall(struct comparameter * parameter)
{
	uint32_t index=0;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_file &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL)
	{
		if (*parameter->paramValue.stringValue != 0)
		{
			index = atoi(parameter->paramValue.stringValue);
			if (index < 0 || index >= numports)
			{
				return EINVAL;
			}
			else
			{
				return print_port(parameter->data1.file_descriptor, index, 1);
			}
		}
		else
		{
			for (index = 0; index < numports; index++)
			{
				print_port(parameter->data1.file_descriptor, index, 1);
			}
			return 0;
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

//int vlancreate(int vlan)
int vlancreate(struct comparameter* parameter)
{
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		if (parameter->paramValue.intValue > 0 && parameter->paramValue.intValue < NUMOFVLAN - 1)
		{ /*vlan NOVLAN (0xfff a.k.a. 4095) is reserved */
			if (bac_check(validvlan, parameter->paramValue.intValue))
			{
				return EEXIST;
			}
			else
			{
				return vlancreate_nocheck(parameter->paramValue.intValue);
			}
		}
		else
		{
			return EINVAL;
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

//int vlanremove(int vlan)
int vlanremove(struct comparameter* parameter)
{
	int index = 0;
	int is_used = 0;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (vlant == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		if (parameter->paramValue.intValue >= 0 && parameter->paramValue.intValue < NUMOFVLAN)
		{
			if (bac_check(validvlan, parameter->paramValue.intValue))
			{

				ba_FORALL(1, vlant[parameter->paramValue.intValue].table, numports, is_used++, index);
				if (is_used)
				{
					return EADDRINUSE;
				}
				else
				{
					bac_clr(validvlan, NUMOFVLAN, parameter->paramValue.intValue);
					free(vlant[parameter->paramValue.intValue].table);
					free(vlant[parameter->paramValue.intValue].bctag);
					free(vlant[parameter->paramValue.intValue].bcuntag);
					free(vlant[parameter->paramValue.intValue].notlearning);
					vlant[parameter->paramValue.intValue].table = NULL;
					vlant[parameter->paramValue.intValue].bctag = NULL;
					vlant[parameter->paramValue.intValue].bcuntag = NULL;
					vlant[parameter->paramValue.intValue].notlearning = NULL;
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
	else
	{
		errno = EINVAL;
		return -1;
	}
}

int vlanaddport(struct comparameter* parameter)
{
	uint32_t port = 0;
	int vlan = 0;
	if (!parameter) {
		errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 = com_type_null &&
		parameter->paramType == com_param_type_string && 
		parameter->paramValue.stringValue != NULL)
	{
		if (sscanf_s(parameter->paramValue.stringValue, "%i %i", &vlan, &port) != 2)
		{
			return EINVAL;
		}
		if (vlan < 0 || vlan >= NUMOFVLAN - 1 || port < 0 || port >= numports)
		{
			return EINVAL;
		}
		if (!bac_check(validvlan, vlan) || portv[port] == NULL)
		{
			return ENXIO;
		}
		if (portv[port]->ep != NULL && portv[port]->vlanuntag != vlan)
		{
			/* changing active port*/
			ba_set(vlant[vlan].bctag, port);
#ifdef FSTP
			fstaddport(vlan, port, 1);
#endif
		}
		ba_set(vlant[vlan].table, port);
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int vlandelport(struct comparameter* parameter)
{
	uint32_t port = 0;
	int vlan = 0;
	if (!parameter) {
		errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 = com_type_null &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL)
	{
		if (sscanf_s(parameter->paramValue.stringValue, "%i %i", &vlan, &port) != 2)
		{
			return EINVAL;
		}
		if (vlan < 0 || vlan >= NUMOFVLAN - 1 || port < 0 || port >= numports)
		{
			return EINVAL;
		}
		if (!bac_check(validvlan, vlan) || portv[port] == NULL)
		{
			return ENXIO;
		}
		if (portv[port]->vlanuntag == vlan)
		{
			return EADDRINUSE;
		}
		if (portv[port]->ep != NULL)
		{
			/*changing active port*/
			ba_clr(vlant[vlan].bctag, port);
#ifdef FSTP
			fstdelport(vlan, port);
#endif
		}
		ba_clr(vlant[vlan].table, port);
		hash_delete_port(port);
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}


//int vlanprint(FILE* fd, char* arg)
int vlanprint(struct comparameter* parameter)
{
	int vlan = 0;
	if (!parameter)
	{
		errno = -1;
		return -1;
	}
	if (
		parameter->type1 = com_type_null &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL)
	{
		if (*parameter->paramValue.stringValue != 0)
		{
			vlan = atoi(parameter->paramValue.stringValue);
			if (vlan >= 0 && vlan < NUMOFVLAN - 1)
			{
				if (bac_check(validvlan, vlan))
				{
					vlanprintactive(vlan, parameter->data1.file_descriptor);
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
		else
		{
			bac_FORALLFUN(1, validvlan, NUMOFVLAN, vlanprintactive, parameter->data1.file_descriptor);
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

//int vlanprintall(FILE* fd, char* arg)
int vlanprintall(struct comparameter* parameter)
{
	if (!parameter)
	{
		errno = -1;
		return -1;
	}
	if (
		parameter->type1 = com_type_null &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL)
	{
		if (*parameter->paramValue.stringValue != 0) {
			int vlan;
			vlan = atoi(parameter->paramValue.stringValue);
			if (vlan > 0 && vlan < NUMOFVLAN - 1)
			{
				if (bac_check(validvlan, vlan))
				{
					vlanprintelem(vlan, parameter->data1.file_descriptor);
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
		else
		{
			bac_FORALLFUN(1, validvlan, NUMOFVLAN, vlanprintelem, parameter->data1.file_descriptor);
		}
	}
	else
	{
		errno = -1;
		return -1;
	}
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
	uint32_t index = portno;
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
#if defined(DEBUGOPT)
//				DBGOUT(DBGPORTNEW, "%02d", index);
				EVENTOUT(DBGPORTNEW, index);
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

int close_ep_port_fd(uint32_t portno, SOCKET fd_ctl)
{
	int rv = 0;
	struct port* port = NULL;
	if (portno >= 0 && portno < numports) {
		port = portv[portno];
		if (port != NULL) {
			rv = rec_close_ep(&(port->ep), fd_ctl);
			if (port->ep == NULL) {
#if defined(DEBUGOPT)
				//DBGOUT(DBGPORTDEL, "%02d", portno);
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


int rec_close_ep(struct endpoint** pep, SOCKET fd_ctl)
{
	struct endpoint* this = *pep;
	if (this != NULL) {
		if (this->fd_ctl == fd_ctl) {
#if defined(DEBUGOPT)
			//DBGOUT(DBGEPDEL, "Port %02d FD %2d", this->port, fd_ctl);
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

int close_ep(struct endpoint* ep)
{
	return close_ep_port_fd(ep->port, ep->fd_ctl);
}

int ep_get_port(struct endpoint* ep)
{
	return ep->port;
}

void handle_in_packet(struct endpoint* ep, struct packet* packet, int len)
{
	int tarport;
	int vlan, tagged;
	int port = ep->port;
	uint32_t index;
	int last;
	void* tmpbuf = NULL;
	void* tmpbuft = NULL;
	void* tmpbufu = NULL;
	unsigned int __ifa1;
	unsigned int __jfa1;
	bitarrayelem __vfa1;
	uint32_t maxfa1;

	//DG minimum length of a packet is 60 bytes plus trailing CRC
	if (len < 60)
	{
		memset(packet->data + len, 0, 60 - len);
		len = 60;
	}
	if(packetfilter(dl+5,port,(char*)packet,len))
	//if (PACKETFILTER(PKTFILTIN, port, packet, len))
	{
#ifdef PORTCOUNTERS
		portv[port]->pktsin++;
		portv[port]->bytesin += len;
#endif
		if (pflag & HUB_TAG)
		{ /* this is a HUB */
#if !defined(VDE_PQ2)
			for (index = 1; index < numports; index++)
			{
				if ((index != port) && (portv[index] != NULL))
				{
					SEND_PACKET_PORT(portv[index], index, (char*)packet, len);
				}
			}
#else
			for (index = 1; index < numports; index++)
			{
				if ((index != port) && (portv[index] != NULL))
				{
					SEND_PACKET_PORT(portv[index], index, packet, len, &tmpbuf);
				}
			}
#endif
		}
		else
		{ /* This is a switch, not a HUB! */
			if (packet->header.proto[0] == 0x81 && packet->header.proto[1] == 0x00)
			{
				tagged = 1;
				vlan = ((packet->data[0] << 8) + packet->data[1]) & 0xfff;
				if (!ba_check(vlant[vlan].table, port))
				{
					return; /*discard unwanted packets*/
				}
			}
			else
			{
				tagged = 0;
				if ((vlan = portv[port]->vlanuntag) == NOVLAN)
				{
					return; /*discard unwanted packets*/
				}
			}

#if defined(FSTP)
			/* when it works as a HUB or FSTP is off, MST packet must be forwarded */
			if (ISBPDU(packet) && fstflag(P_GETFLAG, FSTP_TAG))
			{
				fst_in_bpdu(port, packet, len, vlan, tagged);
				return; /* BPDU packets are not forwarded */
			}
#endif
			/* The port is in blocked status, no packet received */
			if (ba_check(vlant[vlan].notlearning, port))
			{
				return;
			}

			/* We don't like broadcast source addresses */
			if (!(IS_BROADCAST(packet->header.src)))
			{

				last = find_in_hash_update(packet->header.src, vlan, port);
				/* old value differs from actual input port */
				if (last >= 0 && (port != last))
				{
					printlog(LOG_INFO, "MAC %02x:%02x:%02x:%02x:%02x:%02x moved from port %d to port %d", packet->header.src[0], packet->header.src[1], packet->header.src[2], packet->header.src[3], packet->header.src[4], packet->header.src[5], last, port);
				}
			}
			/* static void send_dst(int port,struct packet *packet, int len) */
			if (IS_BROADCAST(packet->header.dest) ||
				(tarport = find_in_hash(packet->header.dest, vlan)) < 0)
			{
				/* FST HERE! broadcast only on active ports*/
				/* no cache or broadcast/multicast == all ports *except* the source port! */
				/* BROADCAST: tag/untag. Broadcast the packet untouched on the ports
				 * of the same tag-ness, then transform it to the other tag-ness for the others*/
				if (tagged)
				{
#if !defined(VDE_PQ2)
					maxfa1 = __WORDSIZEROUND(numports);
					for (__ifa1 = 0; __ifa1 < maxfa1; __ifa1++)
					{
						for (__vfa1 = vlant[vlan].bctag[__ifa1], __jfa1 = 0; __jfa1 < __VDEWORDSIZE; __vfa1 >>= 1, __jfa1++)
						{
							if (__vfa1 & 1) { 
								(index) = __ifa1 * __VDEWORDSIZE + __jfa1; 
								if (index != port)
								{
									SEND_PACKET_PORT(portv[index], index, (char*)packet, len);
								}
							}
						}
					}

					packet = TAG2UNTAG(packet, len);
					maxfa1 = __WORDSIZEROUND(numports);
					for (__ifa1 = 0; __ifa1 < maxfa1; __ifa1++)
					{
						for (__vfa1 = vlant[vlan].bcuntag[__ifa1], __jfa1 = 0; __jfa1 < __VDEWORDSIZE; __vfa1 >>= 1, __jfa1++)
						{
							if (__vfa1 & 1) {
								(index) = __ifa1 * __VDEWORDSIZE + __jfa1;
								if (index != port)
								{
									SEND_PACKET_PORT(portv[index], index, (char*)packet, len);
								}
							}
						}
					}
#else
					
					ba_FORALL(3,vlant[vlan].bctag, numports,
						({ if (index != port) SEND_PACKET_PORT(portv[index],index,packet,len,&tmpbuft); }), index);
					packet = TAG2UNTAG(packet, len);
					ba_FORALL(4,vlant[vlan].bcuntag, numports,
						({ if (index != port) SEND_PACKET_PORT(portv[index],index,packet,len,&tmpbufu); }), index);
#endif
				}
				else
				{ /* untagged */
#if !defined(VDE_PQ2)
					maxfa1 = __WORDSIZEROUND(numports);
					for (__ifa1 = 0; __ifa1 < maxfa1; __ifa1++)
					{
						for (__vfa1 = vlant[vlan].bcuntag[__ifa1], __jfa1 = 0; __jfa1 < __VDEWORDSIZE; __vfa1 >>= 1, __jfa1++)
						{
							if (__vfa1 & 1) {
								(index) = __ifa1 * __VDEWORDSIZE + __jfa1;
								if (index != port)
								{
									SEND_PACKET_PORT(portv[index], index, (char*)packet, len);
								}
							}
						}
					}

					packet = UNTAG2TAG(packet, vlan, len);
					maxfa1 = __WORDSIZEROUND(numports);
					for (__ifa1 = 0; __ifa1 < maxfa1; __ifa1++)
					{
						for (__vfa1 = vlant[vlan].bctag[__ifa1], __jfa1 = 0; __jfa1 < __VDEWORDSIZE; __vfa1 >>= 1, __jfa1++)
						{
							if (__vfa1 & 1) {
								(index) = __ifa1 * __VDEWORDSIZE + __jfa1;
								if (index != port)
								{
									SEND_PACKET_PORT(portv[index], index, (char*)packet, len);
								}
							}
						}
					}
#else
					ba_FORALL(7,vlant[vlan].bcuntag, numports,
						({ if (index != port) SEND_PACKET_PORT(portv[index],index,packet,len,&tmpbufu); }), index);
					packet = UNTAG2TAG(packet, vlan, len);
					ba_FORALL(8,vlant[vlan].bctag, numports,
						({ if (index != port) SEND_PACKET_PORT(portv[index],index,packet,len,&tmpbuft); }), index);
#endif
				}
			}
			else
			{
				/* the hash table should not generate tarport not in vlan
				 * any time a port is removed from a vlan, the port is flushed from the hash */
				if (tarport == port)
				{
					return; /*do not loop!*/
				}
#ifndef VDE_PQ2
				if (tagged)
				{
					if (portv[tarport]->vlanuntag == vlan)
					{ /* TAG->UNTAG */
						packet = TAG2UNTAG(packet, len);
						SEND_PACKET_PORT(portv[tarport], tarport, (char*)packet, len);
					}
					else
					{                               /* TAG->TAG */
						SEND_PACKET_PORT(portv[tarport], tarport, (char*)packet, len);
					}
				}
				else {
					if (portv[tarport]->vlanuntag == vlan)
					{ /* UNTAG->UNTAG */
						SEND_PACKET_PORT(portv[tarport], tarport, (char*)packet, len);
					}
					else
					{                              /* UNTAG->TAG */
						packet = UNTAG2TAG(packet, vlan, len);
						SEND_PACKET_PORT(portv[tarport], tarport, (char*)packet, len);
					}
				}
#else
				if (tagged)
				{
					if (portv[tarport]->vlanuntag == vlan)
					{ /* TAG->UNTAG */
						packet = TAG2UNTAG(packet, len);
						SEND_PACKET_PORT(portv[tarport], tarport, packet, len, &tmpbuf);
					}
					else
					{                               /* TAG->TAG */
						SEND_PACKET_PORT(portv[tarport], tarport, packet, len, &tmpbuf);
					}
				}
				else {
					
					if (portv[tarport]->vlanuntag == vlan)
					{ /* UNTAG->UNTAG */
						SEND_PACKET_PORT(portv[tarport], tarport, packet, len, &tmpbuf);
					}
					else
					{ /* UNTAG->TAG */
						packet = UNTAG2TAG(packet, vlan, len);
						SEND_PACKET_PORT(portv[tarport], tarport, packet, len, &tmpbuf);
					}
				}
#endif
			} /* if(BROADCAST) */
		} /* if(HUB) */
	} /* if(PACKETFILTER) */
}

void setup_description(struct endpoint* ep, char* descr)
{
	//DBGOUT(DBGPORTDESCR, "Port %02d FD %2d -> \"%s\"", ep->port, ep->fd_ctl, descr);
	EVENTOUT(DBGPORTDESCR, ep->port, ep->fd_ctl, descr);
	ep->descr = descr;
}

/* initialize a new endpoint */
struct endpoint* setup_ep(int portno, SOCKET fd_ctl, SOCKET fd_data, uint32_t user, struct mod_support* modfun)
{
	struct port* port;
	struct endpoint* ep;
	unsigned int __ifa1;
	unsigned int __jfa1;
	bitarrayelem __vfa1;
	int __nfa1;
	if ((portno = alloc_port(portno)) >= 0)
	{
		port = portv[portno];
		if (port->ep == NULL && checkport_ac(port, user) == 0)
		{
			port->curuser = user;
		}
		if (port->curuser == user &&
			(ep = malloc(sizeof(struct endpoint))) != NULL)
		{
			//DBGOUT(DBGEPNEW, "Port %02d FD %2d", portno, fd_ctl);
			EVENTOUT(DBGEPNEW, portno, fd_ctl);
			port->ms = modfun;
			port->sender = modfun->sender;
			ep->port = portno;
			ep->fd_ctl = fd_ctl;
			ep->fd_data = fd_data;
			ep->descr = NULL;
#ifdef VDE_PQ2
			ep->vdepq = NULL;
			ep->vdepq_count = 0;
			ep->vdepq_max = stdqlen;
#endif
			if (port->ep == NULL)
			{/* WAS INACTIVE */
				int i;
				/* copy all the vlan defs to the active vlan defs */
				ep->next = port->ep;
				port->ep = ep;
				
				__nfa1 = validvlan[__WORDSIZEROUND(NUMOFVLAN)];
				for (__ifa1 = 0; __nfa1 > 0; __ifa1++)
				{
					for (__vfa1 = (validvlan)[__ifa1], __jfa1 = 0; __jfa1 < __VDEWORDSIZE; __vfa1 >>= 1, __jfa1++)
					{
						if (__vfa1 & 1)
						{
							i = __ifa1 * __VDEWORDSIZE + __jfa1;
							if (ba_check(vlant[i].table, portno)) {
								ba_set(vlant[i].bctag, portno);
#ifdef FSTP
								fstaddport(i, portno, (i != port->vlanuntag));
#endif
							}
							__nfa1--;
						}
					}
				}
				if (port->vlanuntag != NOVLAN) {
					ba_set(vlant[port->vlanuntag].bcuntag, portno);
					ba_clr(vlant[port->vlanuntag].bctag, portno);
					ba_clr(vlant[port->vlanuntag].notlearning, portno);
				}
			}
			else
			{
				ep->next = port->ep;
				port->ep = ep;
			}
			return ep;
		}
		else
		{
			if (port->curuser != user)
			{
				errno = EADDRINUSE;
			}
			else
			{
				errno = ENOMEM;
			}
			return NULL;
		}
	}
	else
	{
		errno = ENOMEM;
		return NULL;
	}
}

int checkport_ac(struct port* port, uint32_t user)
{
	/*unrestricted*/
	if (port->user == -1 && port->group == -1)
		return 0;
	/*root or restricted to a specific user*/
	else if (user == 0 || (port->user != -1 && port->user == user))
		return 0;
	/*restricted to a group*/
	else if (port->group != -1 && UserIsInGroup(user, port->group)==1)
		return 0;
	else {
		errno = EPERM;
		return -1;
	}
}