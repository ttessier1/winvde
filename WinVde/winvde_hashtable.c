#include <libwinvde.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <time.h>

struct winvde_hash_table
{
	size_t payload_size;
	uint32_t hash_mask;
	uint64_t seed;
	char hash_table[1];
};

struct hash_table_element
{
	uint64_t edst;
	time_t last_seen;
	char payload[];
};

#define sizeof_hashtable_element(payload_size)(sizeof(struct hash_table_element)+payload_size)

static inline struct hash_table_element* hash_table_get(struct winvde_hash_table* table, int index)
{
	if (table != NULL)
		return (void*)(table->hash_table + index * sizeof_hashtable_element(table->payload_size));
	else
		return NULL;
}


static inline int calculate_hash(uint64_t src, uint32_t hash_mask, uint64_t seed)
{
	src ^= src >> 33 ^ seed;
	src *= 0xff51afd7ed558ccd;
	src ^= src >> 33;
	src *= 0xc4ceb9fe1a85ec53;
	src ^= src >> 33;
	return src & hash_mask;
}

#define extmac(mac,vlan)\
	(*(uint32_t*)&((mac)[0])+((uint64_t)((*(uint16_t*)&((mac)[4]))+((uint64_t)(vlan)<<16))<<32))

void* winvde_find_in_hash(struct winvde_hash_table* table, unsigned char* destination, int vlan, time_t too_old)
{
	if (table == NULL || destination == NULL )
	{
		return NULL;
	}
	else
	{
		uint64_t edst;
		int index;
		struct hash_table_element * entry;
		/*if ((destination[0] & 1) == 1) // dont worry abour broadcast right now
		{
			return NULL;
		}*/
		edst = extmac(destination, vlan);
		index = calculate_hash(edst, table->hash_mask, table->seed);
		entry = hash_table_get(table, index);
		if (entry != NULL)
		{
			if (entry->edst == edst && entry->last_seen > too_old)
			{
				return &(entry->payload);
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
}

void winvde_find_in_hash_update(struct winvde_hash_table *table, unsigned char * src, int vlan, void * payload, time_t now)
{
	if (table == NULL||src == NULL)
	{
		return ;
	}
	else
	{
		uint64_t esrc;
		int index;
		struct hash_table_element*entry;

		if ((src[0] & 1) == 1)
		{
			return;
		}
		esrc = extmac(src, vlan);
		index = calculate_hash(esrc, table->hash_mask, table->seed);
		entry = hash_table_get(table, index);
		if (entry != NULL)
		{
			entry->edst = esrc;
			memcpy(&(entry->payload), payload, table->payload_size);
			entry->last_seen = now;
		}
		else
		{
			return;
		}

	}
}

void winvde_hash_delete(struct winvde_hash_table * table, void * payload)
{
	if (table == NULL)
	{
		return;
	}
	unsigned int index;
	for (index = 0; index < table->hash_mask + 1; index++)
	{
		struct hash_table_element* entry = hash_table_get(table, index);
		if (memcpy(entry->payload, payload, table->payload_size) == 0)
		{
			entry->last_seen = 0;
		}
	}
}

struct winvde_hash_table* _winvde_hash_table_init(size_t payload_size, uint32_t hashsize, uint64_t seed)
{
	struct winvde_hash_table* retval = NULL;
	if (hashsize == 0)
	{
		return NULL;
	}
	hashsize = (2 << (sizeof(hashsize) * 8 - (hashsize - 1) - 1));
	retval = (struct winvde_hash_table*)malloc(sizeof(struct winvde_hash_table) + hashsize * sizeof_hashtable_element(payload_size));
	if (retval) {
		retval->payload_size = payload_size;
		retval->hash_mask = hashsize - 1;
		retval->seed = seed;
	}
	return retval;
}

void winvde_hash_table_finalize(struct winvde_hash_table* table)
{
	if (table != NULL)
	{
		free(table);
	}
}

#if defined (__UNDEFINED__)
int main()
{
	struct winvde_has_htable* ht = vde_hash_init(int, 15, 0);
	while (1)
	{
		unsigned char mac[7];
		unsigned int port;
		scanf("%6s%u", nmac, &port);
		printf("%s %d \n", mac, port);
		if (port == 0) {
			int* pport = vde_find_in_hash(ht, mac, 0, time(NULL) - 20);
			if (pport)
			{
				printf("-> %d\n", *pport);
			}
			else
			{
				printf("-> not found\n");
			}
		}
		else
		{
			vde_find_in_hash_update(ht, mac, 0, &port, time(NULL));
		}
	}
}

#endif