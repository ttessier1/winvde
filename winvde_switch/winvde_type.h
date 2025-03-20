#pragma once

#include "winvde_module.h"

unsigned char add_type(struct winvde_module* mgr, int prio);
void del_type(unsigned char type);