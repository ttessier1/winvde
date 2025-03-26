#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "winvde_packetq.h"

#if defined(VDE_PQ2)



int vdepq_add(struct vdepq** tail, void* packet, int len, void** tmp)
{
	struct packetbuf* packetbuftmp = *tmp;
	struct vdepq* newelem;
	if ((newelem = malloc(sizeof(struct vdepq))) == NULL)
		return 0;
	if (packetbuftmp == NULL) {
		if ((*tmp = packetbuftmp = malloc(sizeof(struct packetbuf) + len)) == NULL) {
			free(newelem);
			return 0;
		}
		packetbuftmp->len = len;
		packetbuftmp->count = 0;
		memcpy(((void*)(packetbuftmp + 1)), packet, len);
	}
	newelem->vdepq_pb = packetbuftmp;
	(packetbuftmp->count)++;
	//printf("add %p count %d len %d/%d \n",newelem,packetbuftmp->count,len,packetbuftmp->len);
	if (*tail == NULL)
		*tail = newelem->vdepq_next = newelem;
	else {
		newelem->vdepq_next = (*tail)->vdepq_next;
		(*tail)->vdepq_next = newelem;
		*tail = newelem;
	}
	return 1;
}

#define PACKETBUFDEL(X) \
	if (--((X)->count) == 0) \
	 free(X);\
	

void vdepq_del(struct vdepq** tail)
{
	while (*tail != NULL)
	{
		struct vdepq* first = (*tail)->vdepq_next;
		//printf("kill one %p %p\n",first,*tail);
		if (--first->vdepq_pb->count == 0)
		{
			free(first->vdepq_pb);
		}
		if (first == (*tail))
		{
			*tail = NULL;
		}
		else
		{
			(*tail)->vdepq_next = first->vdepq_next;
		}
		free(first);
	}
}

int vdepq_try(struct vdepq** tail, void* ep,
	int (*sendfun)(void* ep, void* packet, int len)) {
	int sent = 0;
	while (*tail != NULL) {
		struct vdepq* first = (*tail)->vdepq_next;
		//printf("trysend %p len %d\n",first,first->vdepq_pb->len);
		if (sendfun(ep, (void*)(first->vdepq_pb + 1), first->vdepq_pb->len) == -EWOULDBLOCK)
			break;
		else {
			if (--first->vdepq_pb->count == 0)
			{
				free(first->vdepq_pb);
			}
			if (first == (*tail))
			{
				*tail = NULL;
			}
			else
			{
				(*tail)->vdepq_next = first->vdepq_next;
			}
			free(first);
			sent++;
		}
	}
	return sent;
}

#endif
