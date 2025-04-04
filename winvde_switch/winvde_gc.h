#pragma once

#define GC_INTERVAL 2
#define GC_EXPIRE 100

extern int gc_interval;
extern int gc_expire;
extern unsigned int gc_timer_no;

void hash_gc(struct comparameter* parameter);
void gc(struct hash_entry* hash_entry_value, void* now);
int hash_set_gc_interval(struct comparameter* parameter);
int hash_set_gc_expire(struct comparameter* parameter);
int hash_get_gc_interval();
int hash_get_gc_expire();