#include <time.h>
#include <errno.h>

#include "winvde_gc.h"
#include "winvde_hash.h"
#include "winvde_comlist.h"
#include "winvde_qtimer.h"


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

void hash_gc(struct comparameter* parameter)
{
	time_t t = qtime();
	for_all_hash(&gc, parameter);
}

//int hash_set_gc_interval(int interval)
int hash_set_gc_interval(struct comparameter * parameter)
{
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		qtimer_del(gc_timer_no);
		gc_interval = parameter->paramValue.intValue;
		gc_timer_no = qtimer_add(gc_interval, 0, hash_gc, NULL);
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

//int hash_set_gc_expire(int expire)
int hash_set_gc_expire(struct comparameter* parameter)
{
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->paramType == com_param_type_int)
	{
		gc_expire = parameter->paramValue.intValue;
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
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