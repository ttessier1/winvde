#include <winmeta.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "winvde_hash.h"
#include "winvde_comlist.h"
#include "winvde_switch.h"
#include "winvde_qtimer.h"
#include "winvde_gc.h"
#include "winvde_output.h"
#include "winvde_loglevel.h"
#include "winvde_debugcl.h"
#include "winvde_debugopt.h"
#include "winvde_event.h"
#include "winvde_printfunc.h"
#include "winvde_mgmt.h"
#include "winvde_memorystream.h"
#include "winvde_mgmt.h"

#ifdef DEBUGOPT
#define DBGHASHNEW (hash_dl) 
#define DBGHASHDEL (hash_dl+1)
static struct dbgcl hash_dl[] = {
	{"hash/+","hash: new element",D_HASH | D_PLUS},
	{"hash/-","hash: discarded element",D_HASH | D_MINUS},
};
#endif

#define MIN_PERSISTENCE_DFL 3
int min_persistence = MIN_PERSISTENCE_DFL;
#define HASH_INIT_BITS 7
int hash_bits;
int hash_mask;
#define HASH_SIZE (1 << hash_bits)

#if BYTE_ORDER == LITTLE_ENDIAN
#define EMAC2MAC6(X) \
	(uint32_t)((X)&0xff), (uint32_t)(((X)>>8)&0xff), (uint32_t)(((X)>>16)&0xff), \
(uint32_t)(((X)>>24)&0xff), (uint32_t)(((X)>>32)&0xff), (uint32_t)(((X)>>40)&0xff)
#elif BYTE_ORDER == BIG_ENDIAN
#define EMAC2MAC6(X) \
	(uint32_t)(((X)>>24)&0xff), (uint32_t)(((X)>>16)&0xff), (uint32_t)(((X)>>8)&0xff), \
  (uint32_t)((X)&0xff), (uint32_t)(((X)>>40)&0xff), (uint32_t)(((X)>>32)&0xff)
#else
#error Unknown Endianess
#endif

#define EMAC2VLAN(X) ((uint16_t) ((X)>>48))
#define EMAC2VLAN2(X) ((uint32_t) (((X)>>48) &0xff)), ((uint32_t) (((X)>>56) &0xff))

#define extmac(MAC,VLAN) \
	    ((*(uint32_t *) &((MAC)[0])) + ((uint64_t) ((*(uint16_t *) &((MAC)[4]))+ ((uint64_t) (VLAN) << 16)) << 32))



struct hash_entry** hash_head;


int po2round(int vx);
int showinfo(struct comparameter* parameter);
int print_hash(struct comparameter* parameter);
int print_hash_entry(struct hash_entry* hash_entry_value, struct comparameter* parameter);
int find_hash(struct comparameter* parameter);
int hash_resize(struct comparameter* parameter);
int hash_set_minper(struct comparameter* parameter);
int calc_hash(uint64_t src);
struct hash_entry* find_entry(uint64_t MAC);
void flush_iterator(struct hash_entry* hash_entry_value, void* arg);
void hash_flush();
void HASH_INIT(int BIT);

static struct comlist hash_cl[] = {
	{"hash","============","HASH TABLE MENU",NULL,NOARG},
	{"hash/showinfo","","show hash info",showinfo,NOARG | WITHFILE},
	{"hash/setsize","N","change hash size",hash_resize,INTARG},
	{"hash/setgcint","N","change garbage collector interval",hash_set_gc_interval,INTARG},
	{"hash/setexpire","N","change hash entries expire time",hash_set_gc_expire,INTARG},
	{"hash/setminper","N","minimum persistence time",hash_set_minper,INTARG},
	{"hash/print","","print the hash table",print_hash,NOARG | WITHFILE},
	{"hash/find","MAC [VLAN]","MAC lookup",find_hash,STRARG | WITHFILE},
};




void HashInit(uint32_t hash_size)
{
	HASH_INIT(po2round(hash_size));

	gc_interval = GC_INTERVAL;
	gc_expire = GC_EXPIRE;
	gc_timer_no = qtimer_add(gc_interval,0,hash_gc,NULL);
	ADDCL(hash_cl);
#if defined(DEBUGOPT)
	adddbgcl(sizeof(hash_dl) / sizeof(struct dbgcl), hash_dl);
#endif
}


int po2round(int vx)
{
	if (vx == 0)
	{
		return 0;
	}
	else
	{
		int index = 0;
		int shift = vx - 1;
		while (shift) { shift >>= 1; index++; }
		if (vx != 1 << index)
		{
			printlog(LOG_WARNING, "Hash size must be a power of 2. %d rounded to %d", vx, 1 << index);
		}
		return index;
	}
}


void for_all_hash(void (*f)(struct hash_entry*, struct comparameter*),struct comparameter* param)
{
	int index;
	struct hash_entry* e, * next;

	for (index = 0; index < HASH_SIZE; index++) {
		for (e = hash_head[index]; e; e = next) {
			next = e->next;
			(*f)(e, param);
		}
	}
}


//int showinfo(FILE* fd)
int showinfo(struct comparameter* parameter)
{
	char* tmpBuff = NULL;
	size_t length = 0;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->data1.file_descriptor != NULL)
	{
		printoutc(parameter->data1.file_descriptor, "Hash size %d", HASH_SIZE);
		printoutc(parameter->data1.file_descriptor, "GC interval %d secs", gc_interval);
		printoutc(parameter->data1.file_descriptor, "GC expire %d secs", gc_expire);
		printoutc(parameter->data1.file_descriptor, "Min persistence %d secs", min_persistence);
		return 0;
	}
	else if (parameter->type1 == com_type_socket && parameter->data1.socket != INVALID_SOCKET)
	{
		length = asprintf(&tmpBuff, "Hash size %d\n", HASH_SIZE);
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "GC interval %d secs\n", gc_interval);
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "GC expire %d secs\n", gc_expire);
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "Min persistence %d secs\n", min_persistence);
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		return 0;
	}
	else if (parameter->type1 == com_type_memstream && parameter->data1.mem_stream != NULL)
	{
		length = asprintf(&tmpBuff, "Hash size %d\n", HASH_SIZE);
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "GC interval %d secs\n", gc_interval);
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "GC expire %d secs\n", gc_expire);
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "Min persistence %d secs\n", min_persistence);
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		return 0;
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
}

//int print_hash(FILE* fd)
int print_hash(struct comparameter* parameter)
{
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_file && 
		parameter->data1.file_descriptor != NULL && 
		parameter->paramType==com_param_type_null
	)
	{
		qtime_csenter();
		for_all_hash(print_hash_entry, parameter);
		qtime_csexit();
	}
	else if (
		parameter->type1 == com_type_socket &&
		parameter->data1.socket != INVALID_SOCKET &&
		parameter->paramType == com_param_type_null
		)
	{
		qtime_csenter();
		for_all_hash(print_hash_entry, parameter);
		qtime_csexit();
	}
	else if (
		parameter->type1 == com_type_memstream &&
		parameter->data1.mem_stream != NULL &&
		parameter->paramType == com_param_type_null
		)
	{
		qtime_csenter();
		for_all_hash(print_hash_entry, parameter);
		qtime_csexit();
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
	return 0;
}

int print_hash_entry(struct hash_entry* hash_entry_value, struct comparameter* parameter)
{
	char* tmpBuff = NULL;
	size_t length = 0;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_file && 
		parameter->data1.file_descriptor != NULL
	)
	{
		printoutc(parameter->data1.file_descriptor, "Hash: %04d Addr: %02x:%02x:%02x:%02x:%02x:%02x VLAN %04d to port: %03d  "
			"age %ld secs",
			calc_hash(hash_entry_value->dst),
			EMAC2MAC6(hash_entry_value->dst),
			EMAC2VLAN(hash_entry_value->dst),
			hash_entry_value->port,
			qtime() - hash_entry_value->last_seen);
		return 0;
	}
	else if (
		parameter->type1 == com_type_socket &&
		parameter->data1.socket != INVALID_SOCKET
		)
	{
		length = asprintf(&tmpBuff,"Hash: %04d Addr: %02x:%02x:%02x:%02x:%02x:%02x VLAN %04d to port: %03d  "
			"age %ld secs",
			calc_hash(hash_entry_value->dst),
			EMAC2MAC6(hash_entry_value->dst),
			EMAC2VLAN(hash_entry_value->dst),
			hash_entry_value->port,
			qtime() - hash_entry_value->last_seen);
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			return 0;
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
	}
	else if (
		parameter->type1 == com_type_memstream &&
		parameter->data1.mem_stream != NULL
		)
	{
		length = asprintf(&tmpBuff, "Hash: %04d Addr: %02x:%02x:%02x:%02x:%02x:%02x VLAN %04d to port: %03d  "
			"age %ld secs",
			calc_hash(hash_entry_value->dst),
			EMAC2MAC6(hash_entry_value->dst),
			EMAC2VLAN(hash_entry_value->dst),
			hash_entry_value->port,
			qtime() - hash_entry_value->last_seen);
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			return 0;
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
	switch_errno = EINVAL;
	return -1;
}

/* looks in global hash table 'h' for given address, and return associated
 * port */
int find_in_hash(unsigned char* dst, int vlan)
{
	struct hash_entry* e = find_entry(extmac(dst, vlan));
	if (e == NULL)
	{
		return -1;
	}
	return(e->port);
}

int find_in_hash_update(unsigned char* src, int vlan, int port)
{
	struct hash_entry* e;
	uint64_t esrc = extmac(src, vlan);
	int k = calc_hash(esrc);
	int oldport;
	time_t now;
	for (e = hash_head[k]; e && e->dst != esrc; e = e->next)
	{
		;
	}
	if (e == NULL)
	{
		e = (struct hash_entry*)malloc(sizeof(*e));
		if (e == NULL)
		{
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_WARNING, "Failed to malloc hash entry %s", errorbuff);
			return -1;
		}

		//DBGOUT(DBGHASHNEW, "%02x:%02x:%02x:%02x:%02x:%02x VLAN %02x:%02x Port %d",
		//	EMAC2MAC6(esrc), EMAC2VLAN2(esrc), port);
		EVENTOUT(DBGHASHNEW, esrc);
		e->dst = esrc;
		if (hash_head[k] != NULL)
		{
			hash_head[k]->prev = &(e->next);
		}
		e->next = hash_head[k];
		e->prev = &(hash_head[k]);
		e->port = port;
		hash_head[k] = e;
	}
	oldport = e->port;
	now = qtime();
	if (oldport != port)
	{
		if ((now - e->last_seen) > min_persistence)
		{
			e->port = port;
			e->last_seen = now;
		}
	}
	else
	{
		e->last_seen = now;
	}
	return oldport;
}


//int find_hash(FILE* fd, char* strmac)
int find_hash(struct comparameter* parameter)
{
	int index = 0;
	int maci[ETH_ALEN];
	unsigned char macv[ETH_ALEN];
	unsigned char* mac = macv;
	int rv = -1;
	int vlan = 0;
	size_t length = 0;
	char* tmpBuff = NULL;
	struct hash_entry* hash_entry_value = NULL;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (strchr(parameter->paramValue.stringValue, ':') != NULL)
	{
		rv = sscanf_s(parameter->paramValue.stringValue, "%x:%x:%x:%x:%x:%x %d", maci + 0, maci + 1, maci + 2, maci + 3, maci + 4, maci + 5, &vlan);
	}
	else
	{
		rv = sscanf_s(parameter->paramValue.stringValue, "%x.%x.%x.%x.%x.%x %d", maci + 0, maci + 1, maci + 2, maci + 3, maci + 4, maci + 5, &vlan);
	}
	if (rv < 6)
	{
		return EINVAL;
	}
	else
	{
		for (index = 0; index < ETH_ALEN; index++)
		{
			mac[index] = maci[index];
		}
		hash_entry_value = find_entry(extmac(mac, vlan));
		if (parameter->type1 == com_type_file && 
			parameter->data1.file_descriptor != NULL &&
			parameter->paramType== com_param_type_string && 
			parameter->paramValue.stringValue != NULL)
		{

			if (hash_entry_value == NULL)
			{
				switch_errno = ENODEV;
				return 0;
			}
			else
			{
				printoutc(parameter->data1.file_descriptor, "Hash: %04d Addr: %02x:%02x:%02x:%02x:%02x:%02x VLAN %04d to port: %03d  "
					"age %ld secs",
					calc_hash(hash_entry_value->dst),
					EMAC2MAC6(hash_entry_value->dst),
					EMAC2VLAN(hash_entry_value->dst),
					hash_entry_value->port + 1,
					qtime() - hash_entry_value->last_seen);
				return 0;
			}
		}
		else if (parameter->type1 == com_type_socket &&
			parameter->data1.socket != INVALID_SOCKET &&
			parameter->paramType == com_param_type_string &&
			parameter->paramValue.stringValue != NULL)
		{

			if (hash_entry_value == NULL)
			{
				switch_errno = ENODEV;
				return 0;
			}
			else
			{
				length = asprintf(&tmpBuff,"Hash: %04d Addr: %02x:%02x:%02x:%02x:%02x:%02x VLAN %04d to port: %03d  "
					"age %ld secs",
					calc_hash(hash_entry_value->dst),
					EMAC2MAC6(hash_entry_value->dst),
					EMAC2VLAN(hash_entry_value->dst),
					hash_entry_value->port + 1,
					qtime() - hash_entry_value->last_seen);
				if (length > 0 && tmpBuff)
				{
					send(parameter->data1.socket, tmpBuff, (int)length, 0);
					free(tmpBuff);
				}
				return 0;
			}
		}
		else if (parameter->type1 == com_type_memstream&&
			parameter->data1.mem_stream != NULL &&
			parameter->paramType == com_param_type_string &&
			parameter->paramValue.stringValue != NULL)
		{
			if (hash_entry_value == NULL)
			{
				switch_errno = ENODEV;
				return 0;
			}
			else
			{
				length = asprintf(&tmpBuff, "Hash: %04d Addr: %02x:%02x:%02x:%02x:%02x:%02x VLAN %04d to port: %03d  "
					"age %ld secs",
					calc_hash(hash_entry_value->dst),
					EMAC2MAC6(hash_entry_value->dst),
					EMAC2VLAN(hash_entry_value->dst),
					hash_entry_value->port + 1,
					qtime() - hash_entry_value->last_seen);
				if (length > 0 && tmpBuff)
				{
					write_memorystream(parameter->data1.socket, tmpBuff, length);
					free(tmpBuff);
				}
				return 0;
			}
		}
		else
		{
			switch_errno = EINVAL;
			return -1;
		}
	}
}

//int hash_resize(int hash_size)
int hash_resize(struct comparameter * parameter)
{
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		if (parameter->paramValue.intValue > 0)
		{
			hash_flush();
			qtime_csenter();
			free(hash_head);
			HASH_INIT(po2round(parameter->paramValue.intValue));
			qtime_csexit();
			return 0;
		}
		else
		{
			return EINVAL;
		}
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
}


//int hash_set_minper(int expire)
int hash_set_minper(struct comparameter* parameter)
{
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		min_persistence = parameter->paramValue.intValue;
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
	return 0;
}

int calc_hash(uint64_t src)
{
	src ^= src >> 33;
	src *= 0xff51afd7ed558ccd;
	src ^= src >> 33;
	src *= 0xc4ceb9fe1a85ec53;
	return src & hash_mask;
}

struct hash_entry* find_entry(uint64_t MAC)
{
	struct hash_entry* hash_entry_value = NULL;
	int k = calc_hash(MAC);
	for (;
		hash_entry_value &&
		hash_entry_value->dst != (MAC);
		hash_entry_value = hash_entry_value->next)
	{
		hash_entry_value = &hash_entry_value[k];
	}
	if (hash_entry_value && hash_entry_value->dst == MAC)
	{
		return hash_entry_value;
	}
	else
	{
		return NULL;
	}
}

void delete_hash_entry(struct hash_entry* OLD)
{
#if defined(DEBUGOPT)
	//DBGOUT(DBGHASHDEL, "%02x:%02x:%02x:%02x:%02x:%02x VLAN %02x:%02x Port %d", EMAC2MAC6(OLD->dst), EMAC2VLAN2(OLD->dst), OLD->port);
	EVENTOUT(DBGHASHDEL, OLD->dst);
#endif
	*((OLD)->prev) = (OLD)->next;
	if ((OLD)->next != NULL) (OLD)->next->prev = (OLD)->prev;
	{
		free((OLD));
	}
}

void flush_iterator(struct hash_entry* hash_entry_value, void* arg)
{
	delete_hash_entry(hash_entry_value);
}

void hash_flush()
{
	qtime_csenter();
	for_all_hash(flush_iterator, NULL);
	qtime_csexit();
}

//TODO: dont exit on failure
// save the existing set, try to allocate a new set
// on success copy over and return 0
// on failure return -1 and set switch_errno appropriately
void HASH_INIT(int BIT)
{
	hash_bits = (BIT);
	if (BIT<32)
	{
		hash_mask = (uint32_t)HASH_SIZE - 1;
		if ((hash_head = (struct hash_entry**)calloc((size_t)HASH_SIZE, sizeof(struct hash_entry*))) == NULL) {
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_WARNING, "Failed to malloc hash table %s", errorbuff);
			exit(1);
		}
	}
	else
	{
		printlog(LOG_WARNING, "Too many bits to shifts for 32: %d\n",BIT);
		exit(1);
	}
}

int delete_port_iterator(struct hash_entry* e, struct comparameter* param)
{
	int pport = 0 ;
	if (!param||!e)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		param->type1 == com_type_null &&
		param->type2 == com_type_null &&
		param->paramType == com_param_type_int
		)
	{
		pport = param->paramValue.intValue;
		if (e->port == pport)
		{
			delete_hash_entry(e);
			return 0;
		}
		else
		{
			switch_errno = EINVAL;
			return -1;
		}
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
}

int hash_delete_port(struct comparameter* param)
{
	if (!param)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		param->type1 == com_type_null &&
		param->type2 == com_type_null &&
		param->paramType == com_param_type_int
		)
	{
		qtime_csenter();
		for_all_hash(delete_port_iterator, param);
		qtime_csexit();
		return 0;
	}
	else
	{
		switch_errno = EINVAL;
		return -1;
	}
}


