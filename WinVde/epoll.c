#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <epoll.h>
#include <stdint.h>
#include <errno.h>





struct epoll_file_descriptors os_file_descriptors = { -1,0,NULL,NULL };
int current_descriptor = -1;

int epoll_create(int size)
{
	struct epoll_file_descriptors* ptr = NULL;
	struct epoll_file_descriptors* save = NULL;
	uint32_t index = 0;

	
	ptr = &os_file_descriptors;
	while (ptr->next != NULL)
	{
		ptr = ptr->next;
	}
	ptr->next = (struct epoll_file_descriptors*)malloc(sizeof(struct epoll_file_descriptors));
	if (ptr->next)
	{
		save = ptr;
		ptr = ptr->next;
			
		memset(ptr, 0, sizeof(struct epoll_file_descriptors));
		ptr->file_descriptor = get_next_descriptor();
		ptr->event = (struct epoll_event*)malloc(sizeof(struct epoll_event) * size);
		if (ptr->event != NULL)
		{
			ptr->number_of_events = size;
			ptr->event->events = 0;
			memset(ptr->event, 0, sizeof(struct epoll_event) * size);
			for (index = 0; index < size; index++)
			{
				ptr->event[index].data.file_descriptor = - 1;
			}
			return ptr->file_descriptor;
		}
		else if (ptr)
		{
			free(ptr);
			save->next = NULL;
		}
	}
	return -1;
}

int epoll_wait(int ipfd, struct epoll_event ** events, int max_events, int timout)
{
	struct epoll_file_descriptors* ptr = NULL;
	
	uint32_t index = 0;
	uint32_t read_fds = 0;
	uint32_t write_fds = 0;
	fd_set write_set;
	fd_set read_set;
	int numSocketsSet = 0;
	int numSetSocketsSet = 0;
	if (ipfd > -1 && ipfd <= current_descriptor)
	{


		ptr = get_epoll_descriptor(ipfd);
		if (!events)
		{
			errno = EINVAL;
			return -1;
		}
		if (ptr != NULL)
		{
			if (ptr->event != NULL)
			{
				FD_ZERO(&read_set);
				FD_ZERO(&write_set);
				for (index = 0; index < ptr->number_of_events; index++)
				{
					if (ptr->event[index].data.file_descriptor != -1)
					{
						if (ptr->event[index].events & EPOLLIN)
						{
							read_fds++;
							FD_SET(ptr->event[index].data.u128.ui64[0], &read_set);
						}
						else if (ptr->event[index].events & EPOLLOUT)
						{
							write_fds++;
							FD_SET(ptr->event[index].data.u128.ui64[0], &write_set);
						}
					}
				}
				numSocketsSet = select(read_fds+write_fds, &read_set, &write_set, NULL, NULL);
				if (numSocketsSet == SOCKET_ERROR)
				{
					fprintf(stderr, "Socket Error On Select: %d\n", WSAGetLastError());
				}
				if (numSocketsSet > 0)
				{
					*events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * numSocketsSet);
					if (*events != NULL)
					{
						ptr = get_epoll_descriptor(ipfd);
						for (index = 0; index < ptr->number_of_events; index++)
						{
							if (ptr->event[index].events & EPOLLIN)
							{
								if (FD_ISSET(ptr->event[index].data.file_descriptor, &read_set))
								{
									memcpy(&((*events)[numSetSocketsSet]), &ptr->event[index], sizeof(struct epoll_event));
									numSetSocketsSet++;
								}
							}
							else if (ptr->event[index].events & EPOLLOUT)
							{
								if (FD_ISSET(ptr->event[index].data.file_descriptor, &write_set))
								{
									memcpy(&((*events)[numSetSocketsSet]), &ptr->event[index], sizeof(struct epoll_event));
									numSetSocketsSet++;
								}
							}
						}
						if (numSocketsSet == numSetSocketsSet)
						{
							return numSocketsSet;
						}
						else
						{
							free((*events));
							events = NULL;
							errno = EINVAL;
							return -1;
						}
					}
					else
					{
						errno = EINVAL;
						return -1;
					}
				}
				else
				{
					errno = EINVAL;
					return -1;
				}

			}
			else
			{
				errno = EINVAL;
				return -1;
			}
		}
		else
		{
			errno = EINVAL;
			return -1;
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

int epoll_ctl(int file_descriptor, int op, SOCKET fd, struct epoll_event* event)
{
	struct epoll_file_descriptors* ptr = NULL;
	if (file_descriptor > -1 && file_descriptor <= current_descriptor)
	{
		ptr = get_epoll_descriptor(file_descriptor);
		uint32_t index = 0;
		uint32_t copy_index = 0;
		struct epoll_event* newevents = NULL;
		BOOL found = FALSE;
		if (!event)
		{
			errno = EINVAL;
			return -1;
		}
		if (ptr != NULL)
		{
			switch (op)
			{
			case EPOLL_CTL_ADD:
				for (index = 0; index < ptr->number_of_events; index++)
				{
					if (ptr->event[index].data.file_descriptor == fd)
					{
						found = TRUE;
					}
				}
				if (!found)
				{
					newevents = (struct epoll_event*)malloc(ptr->number_of_events* sizeof(struct epoll_event));
					if (newevents)
					{
						ptr->number_of_events++;
						for (index = 0; index < ptr->number_of_events - 1; index++)
						{
							if (ptr->event[index].data.file_descriptor != fd)
							{
								memcpy(&newevents[index], &ptr->event[index], sizeof(struct epoll_event));
							}
						}
						memcpy(&newevents[index], event, sizeof(struct epoll_event));
						free(ptr->event);
						ptr->event = newevents;
					}
					else
					{
						errno = EINVAL;
						return -1;
					}
				}
				else
				{
					errno = EINVAL;
					return -1;
				}
				break;
			case EPOLL_CTL_DEL:
				if (ptr->number_of_events > 0)
				{
					for (index = 0; index < ptr->number_of_events; index++)
					{
						if (ptr->event[index].data.file_descriptor == fd)
						{
							found = TRUE;
						}
					}
				}
				if (found)
				{
					ptr->number_of_events--;
					if (ptr->number_of_events > 0)
					{
						newevents = (struct epoll_event*)malloc(ptr->number_of_events*sizeof(struct epoll_event));
						if (newevents)
						{
							for (index = 0; index < ptr->number_of_events; index++)
							{
								if (ptr->event[index].data.file_descriptor != fd)
								{

									memcpy(&ptr->event[copy_index], &ptr->event[index], sizeof(struct epoll_event));
									copy_index++;
								}
							}
							free(ptr->event);
							ptr->event = newevents;
						}
						else
						{
							errno = EINVAL;
							return -1;
						}
					}
					else
					{
						free(ptr->event);
						ptr->event = NULL;
					}
				}
				break;
			case EPOLL_CTL_MOD:
				for (index = 0; index < ptr->number_of_events; index++)
				{
					if (ptr->event[index].data.file_descriptor == fd)
					{
						memcpy(&ptr->event[index], event, sizeof(struct epoll_event));
					}
				}
				break;
			}
		}
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int epoll_release(int file_descriptor)
{
	struct epoll_file_descriptors* ptr = NULL;
	struct epoll_file_descriptors* save = NULL;
	if (file_descriptor != -1 && file_descriptor <= current_descriptor)
	{
		ptr = get_epoll_descriptor(file_descriptor);
		if (ptr)
		{
			if (ptr->number_of_events > 0 && ptr->event)
			{
				if (ptr->event)
				{
					free(ptr->event);
					ptr->event = NULL;
				}
				
				ptr->number_of_events = 0;
				save = &os_file_descriptors;
				while (save->next != NULL)
				{
					if (save->next == ptr)
					{
						save->next = ptr->next;
						break;
					}
					save = save->next;
				}
				return 0;
			}
			else
			{
				return -1;
				errno = EINVAL;
			}
		}
		else
		{
			return -1;
			errno = EINVAL;
		}
	}
	else
	{
		return -1;
		errno = EINVAL;
	}
}

int get_next_descriptor()
{
	return ++current_descriptor;
}

struct epoll_file_descriptors* get_epoll_descriptor(int file_descriptor)
{
	struct epoll_file_descriptors* ptr = NULL;
	if (file_descriptor != -1 && file_descriptor <= current_descriptor)
	{
		ptr = &os_file_descriptors;
		while (ptr != NULL)
		{
			if (ptr->file_descriptor == file_descriptor)
			{
				return ptr;
			}
			ptr = ptr->next;
		}
	}
	return ptr;
}

