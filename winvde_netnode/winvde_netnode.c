// winvde_netnode.cpp : Defines the functions for the static library.
//


#include "framework.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <afunix.h>
#include <sys/stat.h>
#include <libwinvde.h>
#include <winvdeplugmodule.h>
#include <libwinvdeplug_netnode.h>
#include <network_defines.h>
#include <io.h>
#include <direct.h>
#include <epoll.h>
#include <sddl.h>
#include <path_helpers.h>
struct winvdeplug_module winvdeplug_switch_ops = {
	.flags = 0,
	.winvde_open_real = winvde_switch_open,
	.winvde_recv = winvde_netnode_recv,
	.winvde_send = winvde_netnode_send,
	.winvde_datafiledescriptor = winvde_netnode_datafd,
	.winvde_ctlfiledescriptor = winvde_netnode_ctlfd,
	.winvde_close = winvde_netnode_close
};


struct winvdeplug_module  winvdeplug_hub_ops = {
	.flags = 0,
	.winvde_open_real = winvde_hub_open,
	.winvde_recv = winvde_netnode_recv,
	.winvde_send = winvde_netnode_send,
	.winvde_datafiledescriptor = winvde_netnode_datafd,
	.winvde_ctlfiledescriptor = winvde_netnode_ctlfd,
	.winvde_close = winvde_netnode_close
};

struct winvdeplug_module winvdeplug_multi_ops = {
	.flags = 0,
	.winvde_open_real = winvde_multi_open,
	.winvde_recv = winvde_netnode_recv,
	.winvde_send = winvde_netnode_send,
	.winvde_datafiledescriptor = winvde_netnode_datafd,
	.winvde_ctlfiledescriptor = winvde_netnode_ctlfd,
	.winvde_close = winvde_netnode_close
};

struct winvdeplug_module  winvdeplug_bonding_ops = {
	.flags = 0,
	.winvde_open_real = winvde_bonding_open,
	.winvde_recv = winvde_netnode_recv,
	.winvde_send = winvde_netnode_send,
	.winvde_datafiledescriptor = winvde_netnode_datafd,
	.winvde_ctlfiledescriptor = winvde_netnode_ctlfd,
	.winvde_close = winvde_netnode_close
};

#define dualint(hi, lo) (((uint64_t)(hi) << 32) | (lo))
#define dualgetlo(x) ((uint32_t) x)
#define dualgethi(x) ((uint32_t) (x >> 32))

#define SWITCH
#define SWITCH_MAGIC 0xfeedface
#define REQBUFLEN 256
#define PORTTAB_STEP 4
#define STDHASHSIZE 256
#define STDEXPIRETIME 120
#define STDMODE 0600
#define STDDIRMODE 02700
#define STDPATH "\\windows\\temp\\winvdenode_"
#define HOSTFAKEFD -1



enum request_type { REQ_NEW_CONTROL, REQ_NEW_PORT0 };

#include <pshpack1.h>
struct request_v3
{
	uint32_t magic;
	uint32_t version;
	enum request_type type;
	struct sockaddr_un sock;
	char description[1];
};


union port_tab_entry
{
	SOCKET file_desc[2];
};

struct winvde_netnode_conn {
	HMODULE handle;
	BOOL opened;
	struct winvdeplug_module module;
	enum winnetnode_type nettype;
	char* path;
	int epfd;
	SOCKET ctl_fd;
	int mode;
	union port_tab_entry* porttab;
	uint32_t porttablength;
	uint32_t porttabmax;
	struct winvde_hash_table* hashtable;
	int expiretime;
	int lastbonding;
};

#include <poppack.h>

SOCKET ctl_in(char* path, SOCKET ctl_fd) {
	struct sockaddr_un addr;
	size_t len;
	SOCKET new;

	len = sizeof(addr);
	if (len > INT_MAX)
	{
		fprintf(stderr, "ctl_in: Invalid size of sockaddr address: %lld\n", len);
		return -1;
	}
	new = accept(ctl_fd, (struct sockaddr*)&addr, (int*) & len);
	return new;
}

static SOCKET conn_in(char* path, SOCKET conn_fd, int mode) {
	char reqbuf[REQBUFLEN + 1];
	struct request_v3* req = (struct request_v3*)reqbuf;
	int len;
	DWORD option = 1;
	len = recv(conn_fd, reqbuf, REQBUFLEN,0);
	/* backwards compatility: type can have embedded port# (type >> 8) */
	if (len > 0 && req->magic == SWITCH_MAGIC && req->version == 3 &&
		(req->type & 0xff) == REQ_NEW_CONTROL) {
		SOCKET data_fd;
		struct sockaddr_un sunc = req->sock;
		struct sockaddr_un sun;
		sun.sun_family = AF_UNIX;
		data_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
		// Set non blocking mode

		if (ioctlsocket(data_fd, FIONBIO, &option) != 0)
		{
			fprintf(stderr, "conn_in: Failed to put socket into non blocking mode: %d\n", WSAGetLastError());
			return -1;
		}


		if (connect(data_fd, (struct sockaddr*)&sunc, sizeof(sunc)) != 0)
		{
			fprintf(stderr, "conn_in: Failed to connect the socket: %d\n", WSAGetLastError());
			return -1;
		}
		snprintf(sun.sun_path, sizeof(sun.sun_path), "%s\\fd-%lld.sock", path, conn_fd);
		_unlink(sun.sun_path);
		if (bind(data_fd, (struct sockaddr*)&sun, sizeof(SOCKADDR_UN)) != 0)
		{
			fprintf(stderr, "conn_in: Failed to bind the socket: %d\n", WSAGetLastError());
			return -1;
		}
		send(conn_fd, (const char *) & sun, sizeof(sun), 0);
		return data_fd;
	}
	return -1;
}

/* get the vlan id from an Ethernet packet */
int eth_vlan(const void* buf, size_t len) {
	uint32_t double_tagid = 0;
	if (len >= sizeof(struct eth_header) + 4) {
		struct eth_header* ethh = (void*)buf;
		uint16_t* vlanhdr = (void*)(ethh + 1);
		if (ntohs(ethh->ethtype) == ETHERTYPE_VLAN)
		{
			return  ntohs(*vlanhdr) & 0x3ff;
		}
		else if (ntohs(ethh->ethtype) == ETHER_TYPE_DVLAN &&
			ntohs(*vlanhdr) == ETHERTYPE_VLAN)
		{
			double_tagid = ntohs(*vlanhdr) & 0x3ff << 16;
			vlanhdr += sizeof(uint16_t);
			double_tagid += ntohs(*vlanhdr) & 0x3FFF;
			return double_tagid;
		}
	}
	return 0;
}

/* get the source MAC from an Ethernet packet */
uint8_t* eth_shost(const void* buf, size_t len) {
	struct eth_header* ethh = (void*)buf;
	return ethh->source;
}

/* get the destination MAC from an Ethernet packet */
uint8_t* eth_dhost(const void* buf, size_t len) {
	struct eth_header* ethh = (void*)buf;
	return ethh->destination;
}

void porttab_add(struct winvde_netnode_conn* conn, SOCKET fd) {
	if (conn->porttablength == conn->porttabmax) {
		int porttabnewmax = conn->porttabmax + PORTTAB_STEP;
		union port_tab_entry* portnewtab = realloc(conn->porttab, porttabnewmax * sizeof(union port_tab_entry));
		if (portnewtab == NULL)
		{
			return;
		}
		conn->porttab = portnewtab;
		conn->porttabmax = porttabnewmax;
	}
	conn->porttab[conn->porttablength++].file_desc[0] = fd;
}

void porttab_del(struct winvde_netnode_conn* conn, SOCKET file_descriptor) {
	uint32_t index;
	for (index = 0; index < conn->porttablength; index++) {
		if (conn->porttab[index].file_desc[0] == file_descriptor)
			break;
	}
	if (index < conn->porttablength) {
		conn->porttablength--;
		if (conn->porttablength > 0)
			conn->porttab[index] = conn->porttab[conn->porttablength];
	}
}

static int64_t datasock_open(char* path/*, int gid, int mode, int dirmode*/) {
	SOCKET connect_fd;
	struct _stat64 stat;
	struct sockaddr_un sun;
	char* sock_path = NULL;
	size_t pathsize = 0;
	int one = 1;
	int rc = 0;
	if ((connect_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == HOSTFAKEFD)
	{
		fprintf(stderr, "Failed to create unix socket: %d\n", WSAGetLastError());
		goto abort;
	}
	/*if (setsockopt(connect_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0)
	{
		fprintf(stderr, "Failed to set socket option SO_REUSEADDR: %d\n", WSAGetLastError());
		goto abort_connect_fd;
	}*/
	if (_stat64(path, &stat) < 0)
	{
		if (errno == ENOENT)
		{
			if (_mkdir(path) < 0)
			{
				fprintf(stderr, "Failed to create path for unix sockets:%d\n", GetLastError());
				goto abort_connect_fd;
			}
		}
		else
		{
			fprintf(stderr, "Failed to verify path for unix sockets:%d\n", GetLastError());
			goto abort_connect_fd;
		}
	}
	
	/*if (gid >= 0 && chown(path, -1, gid) < 0)
		goto abort_mkdir;
	if (dirmode > 0 && chmod(path, dirmode) < 0)
		goto abort_mkdir;*/
	sun.sun_family = AF_UNIX;
	pathsize = strlen(path) + strlen("\\ctl.sock");
	if(pathsize> UNIX_PATH_MAX)
	{
		fprintf(stderr, "Path is too large for unix sockets:%lld %d\n",pathsize,UNIX_PATH_MAX);
		goto abort_mkdir;
	}
	
	sock_path = (char*)malloc(pathsize+1);
	if (!sock_path)
	{
		fprintf(stderr, "Failed to allocate memory for sock path\n");
		goto abort_mkdir;
	}
	snprintf(sock_path, pathsize+1, "%s\\ctl.sock", path);
	strncpy_s(sun.sun_path, sizeof(sun.sun_path), sock_path, pathsize);
	fprintf(stdout, "Binding to unix socket path: %s %lld\n", sock_path, pathsize);
	free(sock_path);
	
	rc = remove(sun.sun_path);
	if (rc != 0)
	{
		rc = WSAGetLastError();
		if (rc != ENOENT) {
			fprintf(stderr, "remove() error: %d\n", rc);
			goto abort_mkdir;
		}
	}
	rc = bind(connect_fd, (const struct sockaddr*)&sun, sizeof(SOCKADDR_UN));
	if ( rc == SOCKET_ERROR)
	{
		fprintf(stderr,"Failed to bind the connect socket: %d\n", WSAGetLastError());
		fprintf(stderr, "Size of SOCKADDR_UN:%lld\n", sizeof(SOCKADDR_UN));
		goto abort_mkdir;
	}
		
	/*if (gid >= 0 && chown(sun.sun_path, -1, gid) < 0)
		goto abort_unlink;
	if (mode > 0 && chmod(sun.sun_path, mode) < 0)
		goto abort_unlink;*/
	if (listen(connect_fd, 15) < 0)
	{
		fprintf(stderr, "Failed to listen on the socket: %d\n", WSAGetLastError());
		goto abort_unlink;
	}
	return connect_fd;
abort_unlink:
	_unlink(sun.sun_path);
abort_mkdir:
	_rmdir(path);
abort_connect_fd:
	closesocket(connect_fd);
abort:
	return -1;
}

void datasock_close(char* path) {
	if (!path)
	{
		return;
	}
	struct sockaddr_un sun;
	sun.sun_family = AF_UNIX;
	snprintf(sun.sun_path, sizeof(sun.sun_path), "%s\\ctl.sock", path);
	_unlink(sun.sun_path);
	_rmdir(path);
}


WINVDECONN* winvde_netnode_open(const char* winvde_url, const char* descr, int interface_version, struct winvde_open_args* open_args, enum netnode_type nettype)
{
	char* path = NULL;
	char user_path[MAX_PATH];
	struct winvde_netnode_conn* newconn = NULL;
	int epfd = -1;
	int ctl_file_descriptor = 0;
	struct epoll_event event = { .events = EPOLLIN };
	char* modestr = NULL;
	char* dirmodestr = NULL;
	char* grpstr = NULL;
	char* hashsizestr = NULL;
	char* hashseedstr = NULL;
	char* expiretimestr = NULL;
	char* mywinvdeurl = NULL;
	struct winvde_parameters parameters[] =
	{
	//	{(char *)"mode",&dirmodestr},
	//	{(char*)"dirmode",&dirmodestr},
		{(char*)"grp",&grpstr},
		{(char*)"hashsize",&hashsizestr},
		{(char*)"hashseed",&hashseedstr},
		{(char*)"expiretime",&expiretimestr},
		{NULL,NULL},
	};
	//int mode = STDMODE;
	//int dirmode = STDDIRMODE;
	char* lpUsernameBuffer = NULL;
	DWORD userNameSize = 0;
	char userpath[MAX_PATH];

	if (!winvde_url || !descr)
	{
		return NULL;
	}
	mywinvdeurl = _strdup(winvde_url);
	if (!mywinvdeurl)
	{
		return NULL;
	}
	path = (char*)mywinvdeurl;

	if (winvde_parsepathparameters((char*)mywinvdeurl, parameters) != 0)
	{
		return NULL;
	}
	if (!path == 0)
	{
		if (GetUserNameA(lpUsernameBuffer, &userNameSize) == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			lpUsernameBuffer = malloc(sizeof(char) * userNameSize);
			if (lpUsernameBuffer)
			{
				if (GetUserNameA(lpUsernameBuffer, &userNameSize))
				{
					snprintf(userpath, MAX_PATH, STDPATH "%s", lpUsernameBuffer);
				}
				else
				{
					return NULL;
				}
				userpath[MAX_PATH - 1] = '\0';
				path = userpath;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		char* filename = basename(path);
		if (filename == NULL)
		{
			return NULL;
		}
		char* dir = dirname(path);
		if (dir == NULL)
		{
			if (filename)
			{
				free(filename);
			}
			return NULL;
		}
		if((path=_fullpath(dir,user_path,MAX_PATH))==NULL)
		{
			if (filename)
			{
				free(filename);
			}
			if (dir)
			{
				free(dir);
			}
			return NULL;
		}
		strncat_s(path, MAX_PATH,"/", MAX_PATH);
		strncat_s(path, MAX_PATH, filename, MAX_PATH);
		if (filename)
		{
			free(filename);
		}
		if (dir)
		{
			free(dir);
		}
	}
	
	/*
	if (grpstr != NULL) {
		size_t bufsize = sysconf_getgr_r_size_max();
		char buf[bufsize];
		struct group grp;
		struct group *result;
		getgrnam_r(grpstr, &grp, buf, bufsize, &result);
		if (result)
			gid = grp.gr_gid;
		else if (isdigit(*grpstr))
			gid = strtol(grpstr, NULL, 0);
	}


	*/

	//if (modestr) { mode = strtol(modestr, NULL, 8, ) & ~0111; }
	//if (dirmodestr) { dirmode = strol(dirmodestr, NULL, 8); }
	//else if (modestr) { dirmode = 02000 | mode | (mode & 0444) >> 2 | (mode & 0222) >> 1; }

	if ((epfd = epoll_create(1)) < 0)
	{
		goto abort;
	}
	if (ctl_file_descriptor = datasock_open(path/*, gid, mode, dirmode*/) < 0)
	{
		goto abort_epoll;
	}
	event.data.u128.ui64[0] = ctl_file_descriptor;
	event.data.u128.ui64[1] = ctl_file_descriptor;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, ctl_file_descriptor, &event)<0)
	{
		goto abort_datasock;
	}
	if ((newconn = (struct winvde_netnode_conn*)malloc(sizeof(struct winvde_netnode_conn))) == NULL)
	{
		errno = ENOMEM;
		goto abort_datasock;
	}
	memset(newconn, 0, sizeof(struct winvde_netnode_conn));
	newconn->nettype = nettype;
	newconn->epfd = epfd;
	newconn->ctl_fd = ctl_file_descriptor;
	//newconn->mode = mode;
	newconn->path = _strdup(path);
	newconn->porttab = NULL;
	newconn->porttablength = newconn->porttabmax = 0;
	newconn->expiretime = expiretimestr ? atoi(expiretimestr) : STDEXPIRETIME;
	newconn->lastbonding = 0;
	if (nettype == SWITCHNODE)
	{
		newconn->hashtable = winvde_hash_init(int, hashsizestr ? atoi(hashsizestr) : STDHASHSIZE, hashseedstr ? atoi(hashseedstr) : 0);
	}
	else
	{
		newconn->hashtable = NULL;
	}
	newconn->opened = TRUE;
	return (WINVDECONN*)newconn;
abort_datasock:
	closesocket(ctl_file_descriptor);
	datasock_close(path);
abort_epoll:
	if (epfd > -1)
	{
		epoll_release(epfd);
		epfd = -1;
		if (newconn != NULL)
		{
			newconn->epfd  = - 1;
		}
	}

abort:
	return NULL;
}

WINVDECONN* winvde_hub_open(const char* winvde_url, const char* descr, int interface_version,struct winvde_open_args* open_args) {
	return winvde_netnode_open(winvde_url, descr, interface_version, open_args, HUBNODE);
}

WINVDECONN* winvde_multi_open(const char* winvde_url, const char* descr, int interface_version, struct winvde_open_args* open_args) {
	return winvde_netnode_open(winvde_url, descr, interface_version, open_args, MULTINODE);
}

WINVDECONN* winvde_switch_open(const char* winvde_url, const char* descr, int interface_version,struct winvde_open_args* open_args) {
	return winvde_netnode_open(winvde_url, descr, interface_version, open_args, SWITCHNODE);
}


WINVDECONN* winvde_bonding_open(const char* winvde_url, const char* descr, int interface_version,struct winvde_open_args* open_args) {
	
	return winvde_netnode_open(winvde_url, descr, interface_version, open_args, BONDINGNODE);
}

size_t winvde_netnode_recv(WINVDECONN* conn, const char * buf, size_t len, int flags)
{
	size_t returnval = 1;
	uint64_t data = 0;
	uint32_t index = 0;
	SOCKET file_descriptor1 = 0;
	SOCKET file_descriptor2 = 0;
	SOCKET conn_file_descriptor = 0;
	SOCKET out_filedescriptor = 0;
	SOCKET data_file_descriptor = 0;
	struct winvde_netnode_conn* winvde_conn;
	if (conn == NULL || buf == NULL)
	{
		return -1;
	}
	winvde_conn = (struct winvde_netnode_conn*)conn;
	struct epoll_event * event=NULL;
	struct epoll_event other_event ;
	size_t retval = 1;
	if (epoll_wait(winvde_conn->epfd, &event, 1, -1) > 0) {

		file_descriptor1 = event->data.u128.ui64[0];
		file_descriptor2 = event->data.u128.ui64[1];
		if (file_descriptor1 == file_descriptor2)
		{
			if (file_descriptor1 == winvde_conn->ctl_fd)
			{
				// ctl in
				SOCKET conn_file_descriptor = ctl_in(winvde_conn->path,winvde_conn->ctl_fd);
				if (conn_file_descriptor != INVALID_SOCKET)
				{
					other_event.events = EPOLLIN;
					other_event.data.u128.ui64[0] = winvde_conn->ctl_fd;
					other_event.data.u128.ui64[1] = conn_file_descriptor;
					epoll_ctl(winvde_conn->epfd, EPOLL_CTL_ADD, conn_file_descriptor, &other_event);
				}
			}
			else
			{
				if (len > INT_MAX)
				{
					fprintf(stderr, "winvde_netnode_recv: can not read from file descriptor more than INT_MAX bytes:%lld\n",len);
					errno = EINVAL;
					return -1;
				}
			
				len = recv(file_descriptor1, buf, (int)len, 0);
				//len = _read(file_descriptor1, buf, (unsigned int)len);
				if (len >= sizeof(struct eth_header))
				{
					if (winvde_conn->nettype == MULTINODE || winvde_conn->nettype == BONDINGNODE)
					{
						returnval = len;
					}
					else
					{
						for (index = 0; index < winvde_conn->porttablength; index++)
						{
							out_filedescriptor = winvde_conn->porttab[index].file_desc[1];
							if (out_filedescriptor != file_descriptor1)
							{
								if (len > INT_MAX)
								{
									fprintf(stderr, "winvde_netnode_recv: can not write to file descriptor more than INT_MAX bytes:%lld\n", len);
									errno = EINVAL;
									return -1;
								}
								send(out_filedescriptor, buf, (int)len, 0);
							}
						}
						returnval = len;
					}

				}
			}
		}
	}
	else
	{
		if (file_descriptor2 == winvde_conn->ctl_fd)
		{
			data_file_descriptor = conn_in(winvde_conn->path, file_descriptor1, winvde_conn->mode);
			if (data_file_descriptor != INVALID_SOCKET)
			{
				other_event.events = EPOLLIN;
				other_event.data.u128.ui64[0] = data_file_descriptor;
				other_event.data.u128.ui64[1] = file_descriptor1;

				porttab_add(winvde_conn, other_event.data.u128.ui64[0]);
				epoll_ctl(winvde_conn->epfd, EPOLL_CTL_MOD, file_descriptor1, event);
				other_event.data.u128.ui64[0] = data_file_descriptor;
				other_event.data.u128.ui64[1] = data_file_descriptor;
				epoll_ctl(winvde_conn->epfd, EPOLL_CTL_MOD, data_file_descriptor, event);

			}
			else
			{
				struct sockaddr_un sun;
				if (winvde_conn->hashtable)
				{
					winvde_hash_delete(winvde_conn->hashtable, &file_descriptor2);
				}
				porttab_del(winvde_conn, file_descriptor2);
				epoll_ctl(winvde_conn->epfd, EPOLL_CTL_DEL, file_descriptor1, NULL);
				
				closesocket(file_descriptor1);
				closesocket(file_descriptor2);
				sun.sun_family = AF_UNIX;
				snprintf(sun.sun_path, sizeof(sun.sun_path), "%s\\fd-%lld.sock", winvde_conn->path, file_descriptor1);
				_unlink(sun.sun_path);
			}
		}
	}
	return returnval;
}

/* There is an avent coming from the host */
size_t winvde_netnode_send(WINVDECONN* conn, const char * buf, size_t len, int flags)
{
	uint32_t index = 0;
	SOCKET * out_file_descriptor = NULL;
	SOCKET out_filedescriptor = HOSTFAKEFD;
	SOCKET input_file_descriptor = HOSTFAKEFD;
	time_t now = time(NULL);
	struct winvde_netnode_conn* winvde_conn = NULL;
	if (conn == NULL)
	{
		errno = EINVAL;
		return 0;
	}
	if (len >= sizeof(struct eth_header))
	{
		winvde_conn = (struct winvde_netnode_conn*)conn;
		if (winvde_conn->hashtable!=NULL)
		{
			winvde_find_in_hash_update(
				winvde_conn->hashtable, 
				eth_shost(buf, len), 
				eth_vlan(buf, len), 
				&input_file_descriptor, 
				now);
		}
		if(winvde_conn->hashtable != NULL &&
			(out_file_descriptor  = winvde_find_in_hash(
				winvde_conn->hashtable,
				eth_dhost(buf,len)
				,eth_vlan(buf,len),
				now-winvde_conn->expiretime))!=NULL)
		{
			if (out_file_descriptor != HOSTFAKEFD)
			{
				if (len > INT_MAX)
				{
					fprintf(stderr, "Can't send more lan INT_MAX bytes: %lld\n", len);
					return -1;
				}
				send(out_file_descriptor, buf, (int)len, 0);
			}
		}
		else
		{
			if (winvde_conn->nettype == BONDINGNODE)
			{
				if (winvde_conn->porttablength > 0)
				{
					
					winvde_conn->lastbonding = (winvde_conn->lastbonding + 1) % winvde_conn->porttablength;
					out_filedescriptor = winvde_conn->porttab[winvde_conn->lastbonding].file_desc[1];
					if (len > INT_MAX)
					{
						fprintf(stderr,"can't send more than INT_MAX bytes: %lld\n", len);
						return -1;
					}
					send(out_filedescriptor, buf, (int)len,0);

				}
			}
			else
			{
				for (index = 0; index < winvde_conn->porttablength; index++)
				{
					out_filedescriptor = winvde_conn->porttab[index].file_desc[1];
					if (len > INT_MAX)
					{
						fprintf(stderr, "can't send more than INT_MAX bytes: %lld\n", len);
						return -1;
					}
					send(out_filedescriptor, buf, (int)len, 0);
				}
			}
		}
			
	}
	return len;
}

int winvde_netnode_datafd(WINVDECONN* conn)
{
	if (conn == NULL)
	{
		return -1;
	}
	struct winvde_netnode_conn* winvde_conn = (struct winvde_netnode_conn*)conn;
	return winvde_conn->epfd;
}

int winvde_netnode_ctlfd(WINVDECONN* conn)
{
	fprintf(stderr, "winvde_netnode_ctlfd: Not Implemented\n");
	return -1;
}

int winvde_netnode_close(WINVDECONN* conn)
{
	uint32_t index = 0;
	struct winvde_netnode_conn* winvde_conn = (struct winvde_netnode_conn*)conn;
	if (!conn)
	{
		return -1;
	}
	if (!conn->opened)
	{
		return -1;
	}
	for (index = 0; index < winvde_conn->porttablength; index++)
	{
		SOCKET file_descriptor1 = winvde_conn->porttab[index].file_desc[0];

		SOCKET file_descriptor2 = winvde_conn->porttab[index].file_desc[1];
		struct sockaddr_un sun;
		closesocket(file_descriptor1);
		closesocket(file_descriptor2);
		sun.sun_family = AF_UNIX;
		snprintf(sun.sun_path, sizeof(sun.sun_path), "%s\\fd-%lld.sock", winvde_conn->path, file_descriptor1);
		_unlink(sun.sun_path);
	}
	datasock_close(winvde_conn->path);
	closesocket(winvde_conn->ctl_fd);

	epoll_release(winvde_conn->epfd);
	if (winvde_conn->porttab)
	{
		free(winvde_conn->porttab);
	}
	if (winvde_conn->hashtable)
	{
		winvde_hash_table_finalize(winvde_conn->hashtable);
	}
	if (winvde_conn->path)
	{
		free(winvde_conn->path);
	}
	free(winvde_conn);
	return 0;
}
