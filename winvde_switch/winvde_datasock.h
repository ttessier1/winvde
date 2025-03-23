#pragma once

void StartDataSock(void);
struct endpoint* new_port_v1_v3(SOCKET fd_ctl, int type_port, struct sockaddr_un* sun_out);