#pragma once

void sig_handler(int sig);
void setsighandlers(void(*cleanup)(void));