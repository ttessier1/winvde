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

void for_all_hash(void (*f)(struct hash_entry*, void*), void* arg);
void HashInit(uint32_t hash_size);
void delete_hash_entry(struct hash_entry* OLD);
struct hash_entry* find_entry(uint64_t MAC);
void delete_port_iterator(struct hash_entry* e, void* arg);
void hash_delete_port(int port);