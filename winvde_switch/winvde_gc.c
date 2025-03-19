#include <time.h>


#include "winvde_gc.h"
#include "winvde_hash.h"

int gc_interval = 0;
int gc_expire = 0;
unsigned int gc_timer_no = 0;

void gc(struct hash_entry* hash_entry_value, void* now)
{
	time_t t = *(time_t*)now;
	if (hash_entry_value != NULL)
	{
		if (hash_entry_value->last_seen + gc_expire < t)
		{
			delete_hash_entry(hash_entry_value);
		}
	}
}

void hash_gc(void* arg)
{
	time_t t = qtime();
	for_all_hash(&gc, &t);
}

int hash_set_gc_interval(int interval)
{
	qtimer_del(gc_timer_no);
	gc_interval = interval;
	gc_timer_no = qtimer_add(gc_interval, 0, hash_gc, NULL);
	return 0;
}

int hash_set_gc_expire(int expire)
{
	gc_expire = expire;
	return 0;
}

int hash_get_gc_interval()
{
	return gc_interval;
}

int hash_get_gc_expire()
{
	return gc_expire;
}