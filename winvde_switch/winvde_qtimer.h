#pragma once
#include <time.h>

time_t qtime();
int qtimer_init();
unsigned int qtimer_add(time_t period, int times, void(*call)(), void* args);
void qtimer_del(unsigned int timer_id); 
int qtime_csenter();
void qtime_csexit();