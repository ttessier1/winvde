#pragma once

#include <stdint.h>
#include <time.h>

struct hash_entry {
	struct hash_entry* next;
	struct hash_entry** prev;
	time_t last_seen;
	int port;
	uint64_t dst;
};

void for_all_hash(void (*f)(struct hash_entry*, struct comparameter*), struct comparameter* param);
void HashInit(uint32_t hash_size);
void delete_hash_entry(struct hash_entry* OLD);
struct hash_entry* find_entry(uint64_t MAC);
int delete_port_iterator(struct hash_entry* e, struct comparameter* param);
int hash_delete_port(struct comparameter* param);
int find_in_hash(unsigned char* dst, int vlan);
int find_in_hash_update(unsigned char* src, int vlan, int port);