#pragma once

#if defined(VDE_PQ2)


struct packetbuf {
	short len;
	short count;
};

struct vdepq {
	struct packetbuf* vdepq_pb;
	struct vdepq* vdepq_next;
};

int vdepq_add(struct vdepq** tail, void* packet, int len, void** tmp);
void vdepq_del(struct vdepq** tail);
int vdepq_try(struct vdepq** tail, void* ep,int (*sendfun)(void* ep, void* packet, int len));

#endif