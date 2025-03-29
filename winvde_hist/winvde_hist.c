// winvde_hist.cpp : Defines the functions for the static library.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>

#include "winvde_printfunc.h"
#include "winvde_memorystream.h"
#include "telnet.h"
#include "winvde_hist.h"

#pragma comment(lib,"ws2_32.lib")

int_socket_function vdehist_vderead = recv;
int_socket_function vdehist_vdewrite = send;
int_file_function vdehist_termread = _read;
int_file_function vdehist_termwrite = _write;

char* nologin(char* cmd, int len, struct vdehiststat* st) {
	return NULL;
}

char* (*vdehist_logincmd)(char* cmd, int len, struct vdehiststat* s)
= nologin;

int commonprefix(char* x, char* y, int maxlen)
{
	int len = 0;
	while (*(x++) == *(y++) && len < maxlen)
	{
		len++;
	}
	return len;
}

void showexpand(char* linebuf, int bufindex, int termfd)
{
	char* buf = NULL ;
	char* buf2 = NULL;
	size_t * bufsize = 0 ;
	size_t bufsize2 = 0;
	struct _memory_file* ms = NULL;
	ms = open_memorystream(&buf, &bufsize);
	int nmatches = 0;
	if (ms)
	{
		if (commandlist && bufindex > 0)
		{
			char** s = commandlist;
			while (*s)
			{
				if (strncmp(linebuf, *s, bufindex) == 0)
				{
					nmatches++;
					bufsize2 = asprintf(&buf2, "%s", *s);
					
					if (bufsize2)
					{
						write_memorystream(ms, buf2, bufsize2);
						free(buf2);
					}
				}
				s++;
			}
			
			write_memorystream(ms, "\r\n", strlen("\r\n"));
		}
		get_buffer(ms);
		if (nmatches > 1)
		{
			vdehist_termwrite(termfd, buf, strlength(buf));
			close_memorystream(ms);
		}
		
	}
}

int tabexpand(char* linebuf, int bufindex, int maxlength)
{
	char** s = commandlist;
	int nmatches = 0;
	int len = 0;
	char* match = NULL;
	if (commandlist && bufindex > 0) {
		while (*s)
		{
			if (strncmp(linebuf, *s, bufindex) == 0)
			{
				nmatches++;
				if (nmatches == 1)
				{
					match = *s;
					len = strlength(match);
				}
				else
				{
					len = commonprefix(match, *s, len);
				}
			}
			s++;
		}
		if (len > 0)
		{
			int alreadymatch = commonprefix(linebuf, match, len);
			//fprintf(stderr,"TAB %s %d -> %s %d already %d\n",linebuf,bufindex,match,len,alreadymatch);
			if ((len - alreadymatch) + strlen(linebuf) < maxlength)
			{
				memmove(linebuf + len, linebuf + alreadymatch,
					strlen(linebuf + alreadymatch) + 1);
				memcpy(linebuf + alreadymatch, match + alreadymatch, len - alreadymatch);
				if (nmatches == 1 && linebuf[len] != ' ' && strlen(linebuf) + 1 < maxlength)
				{
					memmove(linebuf + len + 1, linebuf + len,
						strlen(linebuf + len) + 1);
					linebuf[len] = ' ';
					len++;
				}
				bufindex = len;
			}
		}
	}
	return bufindex;
}

int qstrcmp(const void* a, const void* b)
{
	return strcmp(*(char* const*)a, *(char* const*)b);
}

char* vdehist_readln(SOCKET vdefd, char* linebuf, int size, struct vh_readln* vlb)
{
	int i;
	char lastch = ' ';
	int sel = 0;
	LPWSAPOLLFD wsaPollFD = NULL;
	
	wsaPollFD = (LPWSAPOLLFD)malloc(sizeof(WSAPOLLFD) * 2);
	if (!wsaPollFD)
	{
		fprintf(stderr, "Failed to allocate memory for WSAPollFD\n");
		return NULL;
	}
	wsaPollFD[0].fd = vdefd;
	wsaPollFD[0].events = POLLIN;
	wsaPollFD[0].revents = 0;
	wsaPollFD[1].fd = vdefd;
	wsaPollFD[1].events = POLLOUT;
	wsaPollFD[1].revents = 0;
	i = 0;
	do
	{
		if (vlb->readbufindex == vlb->readbufsize)
		{
			sel = WSAPoll(wsaPollFD,2,0);
			if (sel < 0)
			{
				linebuf = NULL;
				fprintf(stderr, "Failed to do WSAPoll on readlin: %d\n",WSAGetLastError());
				break;
			}
			if ((vlb->readbufsize = recv(vdefd, vlb->readbuf, BUFSIZE,0)) <= 0)
			{
				linebuf = NULL;
				break;
			}
			vlb->readbufindex = 0;
		}
		if (vlb->readbuf[vlb->readbufindex] == ' ' && lastch == '$' && vlb->readbufindex == vlb->readbufsize - 1)
		{
			linebuf = NULL;
			goto CleanUp;
		}
		lastch = linebuf[i] = vlb->readbuf[vlb->readbufindex];
		i++; vlb->readbufindex++;
	} while (lastch != '\n' && i < size - 1);
	if (linebuf != NULL)
	{
		linebuf[i] = 0;
	}
CleanUp:
	if (wsaPollFD!=NULL)
	{
		free(wsaPollFD);
		wsaPollFD = NULL;
	}
	return linebuf;
}

int vdehist_create_commandlist(SOCKET vdefd)
{
	int status = 0;
	int readSomeData = 0;
	char linebuf[BUFSIZE];
	struct vh_readln readlnbuf = { 0,0 };
	char* buf;
	char* returnBuf = NULL;
	size_t * bufsize;
	char* lastcommand = NULL;
	int commentCount = 0;
	/* use a memstream to create the array.
		 add (char *) elements by fwrite */
	struct _memory_file* ms = NULL;
	ms = open_memorystream(&buf, &bufsize);
	if (ms && vdefd >= 0)
	{
		status = CC_HEADER;
		vdehist_vdewrite(vdefd, "help\n", 5,0);
		while (status != CC_TERM && vdehist_readln(vdefd, linebuf, BUFSIZE, &readlnbuf) != NULL)
		{
			readSomeData = 1;
			if (status == CC_HEADER)
			{
				if (strncmp(linebuf, "------------", 12) == 0)
				{
					status = CC_BODY;
				}
			}
			else
			{
				if (strncmp(linebuf, ".\n", 2) == 0)
				{
					status = CC_TERM;
				}
				else
				{
					char* s = linebuf;
					while (*s != ' ' && *s != 0)
					{
						s++;
					}
					*s = 0; /* take the first token */
					/* test for menu header */
					if (lastcommand)
					{
						if (strncmp(lastcommand, linebuf, strlen(lastcommand)) == 0 &&
							linebuf[strlen(lastcommand)] == '/')
						{
							free(lastcommand);
						}
						else
						{
							write_memorystream(ms, (char*)&lastcommand, sizeof(char*));
							//fwrite(&lastcommand, sizeof(char*), 1, ms);
							commentCount++;
						}
					}
					lastcommand = _strdup(linebuf);
					
				}
			}
		}
		if (readSomeData == 0 && lastcommand!=NULL)
		{
			return -1;
		}
		if (lastcommand)
		{
			write_memorystream(ms, (char*) & lastcommand, sizeof(char*));
			//fwrite(&lastcommand, sizeof(char*), 1, ms);
		}
		lastcommand = NULL;
		write_memorystream(ms, (char*) & lastcommand, sizeof(char*));
		//fwrite(&lastcommand, sizeof(char*), 1, ms);
		
		returnBuf = malloc(*bufsize);
		if (returnBuf)
		{
			get_buffer(ms);
			memcpy(returnBuf, buf,*bufsize);
			commandlist = (char**)returnBuf;
		}
		qsort(commandlist, commentCount, sizeof(char*), qstrcmp);
		close_memorystream(ms);
	}
	return 0;
}

void erase_line(struct vdehiststat* st, int prompt_too)
{
	int j= 0 ;
	int size = st->bufindex + (prompt_too != 0) * strlength(prompt);
	char* buf = NULL;
	char* buff2 = NULL;

	size_t * bufsize = 0;
	size_t bufsize2 = 0;
	struct _memory_file* ms = NULL;

	ms = open_memorystream(&buf, &bufsize);
	if (ms)
	{
		for (j = 0; j < size; j++)
		{
			writechar_memorystream(ms, '\010');
		}
		size = strlength(st->linebuf) + (prompt_too != 0) * strlength(prompt);
		for (j = 0; j < size; j++)
		{
			writechar_memorystream(ms, ' ');
		}
		for (j = 0; j < size; j++)
		{
			writechar_memorystream(ms, '\010');
		}
		get_buffer(ms);
		if (buf)
		{
			vdehist_termwrite(st->termfd, buf, (int)*bufsize);
			close_memorystream(ms);
		}
	}
}


void redraw_line(struct vdehiststat* st, int prompt_too)
{
	int j;
	int tail = strlength(st->linebuf) - st->bufindex;
	char* buf = NULL;
	char* buff2 = NULL;
	
	size_t * bufsize = 0;
	size_t bufsize2 = 0;
	struct _memory_file* ms = NULL;
	
	ms = open_memorystream(&buf, &bufsize);
	if (ms)
	{
		if (prompt_too)
		{
			bufsize2 = asprintf(&buff2, "\033[32m%.*s%s\033[0m", strlen(prompt),prompt, st->linebuf);
			if (bufsize2 > 0)
			{
				write_memorystream(ms, buff2,bufsize2);
				free(buff2);
				buff2 = NULL;
			}
		}
		else
		{
			bufsize2 = asprintf(&buff2, "\033[32m%.*s\033[0m", prompt);
			if (bufsize2 > 0)
			{
				write_memorystream(ms, buff2, bufsize2);
				free(buff2);
				buff2 = NULL;
			}
		}
		for (j = 0; j < tail; j++)
		{
			writechar_memorystream(ms, '\010');
		}
		get_buffer(ms);
		if (buf)
		{
			vdehist_termwrite(st->termfd, buf, (int)*bufsize);
			
			close_memorystream(ms);
		}
	}
}

int vdehist_mgmt_to_term(struct vdehiststat* st)
{
	char buf[BUFSIZE + 1];
	size_t n = 0;
	int ib = 0;
	/* erase the input line */
	erase_line(st, 1);
	/* if the communication with the manager object holds, print the output*/
	//fprintf(stderr,"mgmt2term\n");
	if (st->mgmtfd)
	{
		n = vdehist_vderead(st->mgmtfd, buf, BUFSIZE,0);
		if (n < 0)
		{
			fprintf(stderr, "Failed to read: %d\n",WSAGetLastError());
			return -1;
		}
		//fprintf(stderr,"mgmt2term n=%d\n",n);
		buf[n] = 0;
		while (n != 0xFFFFFFFFFFFFFFFF && n > 0)
		{
			for (ib = 0; ib < n; ib++)
			{
				st->vlinebuf[(st->vbufindex)++] = buf[ib];
				if (buf[ib] == '\n')
				{
					st->vlinebuf[(st->vbufindex) - 1] = '\r';
					st->vlinebuf[(st->vbufindex)] = '\n';
					st->vlinebuf[(st->vbufindex) + 1] = '\0';
					(st->vbufindex)++;
					if (st->vindata)
					{
						if (st->vlinebuf[0] == '.' && st->vlinebuf[1] == '\r')
						{
							st->vindata = 0;
						}
						else
						{
							vdehist_termwrite(st->termfd, st->vlinebuf, (st->vbufindex));
						}
					}
					else
					{
						char* message = st->vlinebuf;
						//fprintf(stderr,"MSG1 \"%s\"\n",message);
						while (*message != '\0' &&
							!(isdigit(message[0]) &&
								isdigit(message[1]) &&
								isdigit(message[2]) &&
								isdigit(message[3])))
						{
							message++;
						}
						if (strncmp(message, "0000", 4) == 0)
						{
							st->vindata = 1;
						}
						else if (isdigit(message[1]) &&
							isdigit(message[2]) &&
							isdigit(message[3]))
						{
							if (message[0] == '1')
							{
								message += 5;
								vdehist_termwrite(st->termfd, message, strlength(message));
							}
							else if (message[0] == '3')
							{
								message += 5;
								vdehist_termwrite(st->termfd, "** DBG MSG: ", 12);
								vdehist_termwrite(st->termfd, (message), strlength(message));
							}
						}
					}
					(st->vbufindex) = 0;
				}
			}
			n = vdehist_vderead(st->mgmtfd, buf, BUFSIZE,0);
		}
	}
	if (commandlist == NULL && st->mgmtfd >= 0)
	{
		if (vdehist_create_commandlist(st->mgmtfd) == -1)
		{
			return -1;
		}
	}
	/* redraw the input line */
	redraw_line(st, 1);
	return 0;
}

int hist_sendcmd(struct vdehiststat* st)
{
	char* cmd = st->linebuf;
	if (st->status != HIST_COMMAND)
	{
		cmd = vdehist_logincmd(cmd, st->bufindex, st);
		if (commandlist == NULL && st->mgmtfd >= 0)
		{
			vdehist_create_commandlist(st->mgmtfd);
		}
		if (cmd == NULL)
		{
			return 0;
		}
	}
	while (*cmd == ' ' || *cmd == '\t')
	{
		cmd++;
	}
	if (strncmp(cmd, "logout", 6) == 0)
	{
		return 1;
	}
	else
	{
		if (*cmd != 0)
		{
			send(st->mgmtfd, st->linebuf, st->bufindex, 0);
			if (strncmp(cmd, "shutdown", 8) == 0)
			{
				return 2;
			}
		}
		vdehist_termwrite(st->termfd, "\r\n", 2);
		vdehist_termwrite(st->termfd, prompt, strlength(prompt));
	}
	if (commandlist != NULL &&
		(strncmp(cmd, "plugin/add", 10) == 0 ||
			strncmp(cmd, "plugin/del", 10) == 0))
	{
		free(commandlist);
		commandlist = NULL;
	}
	return 0;
}

void put_history(struct vdehiststat* st)
{
	if (st->history[st->histindex])
	{
		free(st->history[st->histindex]);
	}
	st->history[st->histindex] = _strdup(st->linebuf);
}

void get_history(int change, struct vdehiststat* st)
{
	st->histindex += change;
	if (st->histindex < 0)
	{
		st->histindex = 0;
	}
	if (st->histindex >= HISTORYSIZE)
	{
		st->histindex = HISTORYSIZE - 1;
	}
	if (st->history[st->histindex] == NULL)
	{
		(st->histindex)--;
	}
	strcpy_s(st->linebuf, sizeof(st->linebuf), st->history[st->histindex]);
	st->bufindex = strlength(st->linebuf);
}

void shift_history(struct vdehiststat* st)
{
	if (st->history[HISTORYSIZE - 1] != NULL)
	{
		free(st->history[HISTORYSIZE - 1]);
	}
	memmove(st->history + 1, st->history, (HISTORYSIZE - 1) * sizeof(char*));
	st->history[0] = NULL;
}

void telnet_option_send3(int fd, int action, int object)
{
	char opt[3];
	opt[0] = 0xff;
	opt[1] = action;
	opt[2] = object;
	vdehist_termwrite(fd, opt, 3);
}

int telnet_options(struct vdehiststat* st, unsigned char* s)
{
	int action_n_object;
	if (st->telnetprotocol == 0)
	{
		st->telnetprotocol = 1;
		st->echo = 0;
		telnet_option_send3(st->termfd, WILL, TELOPT_ECHO);
	}
	int skip = 2;
	s++;
	action_n_object = ((*s) << 8) + (*(s + 1));
	switch (action_n_object)
	{
	case (DO << 8) + TELOPT_ECHO:
		//printf("okay echo\n");
		st->echo = 1;
		break;
	case (WILL << 8) + TELOPT_ECHO:
		telnet_option_send3(st->termfd, DONT, TELOPT_ECHO);
		telnet_option_send3(st->termfd, WILL, TELOPT_ECHO);
		break;
	case (DO << 8) + TELOPT_SGA:
		//printf("do sga -> okay will sga\n");
		telnet_option_send3(st->termfd, WILL, TELOPT_SGA);
		break;
	case (WILL << 8) + TELOPT_TTYPE:
		//printf("will tty -> dont tty\n");
		telnet_option_send3(st->termfd, DONT, TELOPT_TTYPE);
		break;
	default:
		//printf("not managed yet %x %x\n",*s,*(s+1));
		if (*s == WILL)
		{
			telnet_option_send3(st->termfd, DONT, *(s + 1));
		}
		else if (*s == DO)
		{
			telnet_option_send3(st->termfd, WONT, *(s + 1));
		}
	}
	return skip;
}

size_t vdehist_term_to_mgmt(struct vdehiststat* st,char * buf, int size)
{
	int i = 0;
	int rv = 0;
	if (!buf || size == 0)
	{
		return -1;
	}
	else if (size < 0)
	{
		return size;
	}
	else
	{
		for (i = 0; i < size && strlength(st->linebuf) < size; i++)
		{
			if (buf[i] == 0xff && buf[i + 1] == 0xff)
			{
				i++;
			}
			if (buf[i] == 0)
			{
				buf[i] = '\n'; /*telnet encode \n as a 0 when in raw mode*/
			}
			if (buf[i] == 0xff && buf[i + 1] != 0xff)
			{
				i += telnet_options(st, buf + i);
			}
			else
			{
				if (buf[i] == 0x1b)
				{
					/* ESCAPE! */
					if (buf[i + 1] == '[' && st->status == HIST_COMMAND)
					{
						st->edited = 1;
						switch (buf[i + 2])
						{
						case 'A': //fprintf(stderr,"UP\n");
							erase_line(st, 0);
							put_history(st);
							get_history(1, st);
							redraw_line(st, 0);
							st->bufindex = strlength(st->linebuf);
							break;
						case 'B': //fprintf(stderr,"DOWN\n");
							erase_line(st, 0);
							put_history(st);
							get_history(-1, st);
							redraw_line(st, 0);
							break;
						case 'C': //fprintf(stderr,"RIGHT\n");
							if (st->linebuf[st->bufindex] != '\0') {
								vdehist_termwrite(st->termfd, "\033[C", 3);
								(st->bufindex)++;
							}
							break;
						case 'D': //fprintf(stderr,"LEFT\n");
							if (st->bufindex > 0) {
								vdehist_termwrite(st->termfd, "\033[D", 3);
								(st->bufindex)--;
							}
							break;
						}
						i += 3;
					}
					else
					{
						i += 2;/* ignored */
					}
				}
				else if (buf[i] < 0x20 && !(buf[i] == '\n' || buf[i] == '\r'))
				{
					/*ctrl*/
					if (buf[i] == 4) /*ctrl D is a shortcut for UNIX people! */
					{
						rv = 1;
						break;
					}
					switch (buf[i]) {
					case 3:  /*ctrl C cleans the current buffer */
						erase_line(st, 0);
						st->bufindex = 0;
						st->linebuf[(st->bufindex)] = 0;
						break;
					case 12: /* ctrl L redraw */
						erase_line(st, 1);
						redraw_line(st, 1);
						break;
					case 1: /* ctrl A begin of line */
						erase_line(st, 0);
						st->bufindex = 0;
						redraw_line(st, 0);
						break;
					case 5: /* ctrl E endofline */
						erase_line(st, 0);
						st->bufindex = strlength(st->linebuf);
						redraw_line(st, 0);
					case '\t': /* tab */
						if (st->lastchar == '\t')
						{
							erase_line(st, 1);
							showexpand(st->linebuf, st->bufindex, st->termfd);
							redraw_line(st, 1);
						}
						else
						{
							erase_line(st, 0);
							st->bufindex = tabexpand(st->linebuf, st->bufindex, BUFSIZE);
							redraw_line(st, 0);
						}
						break;
					}
				}
				else if (buf[i] == 0x7f)
				{
					if (st->bufindex > 0)
					{
						char* x;
						(st->bufindex)--;
						x = st->linebuf + st->bufindex;
						memmove(x, x + 1, strlen(x));
						if (st->echo && !(st->status & HIST_PASSWDFLAG))
						{
							if (st->edited)
							{
								vdehist_termwrite(st->termfd, "\010\033[P", 4);
							}
							else
							{
								vdehist_termwrite(st->termfd, "\010 \010", 3);
							}
						}
					}
				}
				else
				{
					if (st->echo && !(st->status & HIST_PASSWDFLAG))
					{
						if (st->edited && buf[i] >= ' ')
						{
							vdehist_termwrite(st->termfd, "\033[@", 3);
						}
						vdehist_termwrite(st->termfd, &(buf[i]), size);
						vdehist_termwrite(st->termfd, "\n",1);
						send(st->mgmtfd, &(buf[i]), size,0);
						
						i += size;
					}
					else if(!st->echo && !(st->status& HIST_PASSWDFLAG))
					{
						send(st->mgmtfd, &(buf[i]), size, 0);
						i += size;
					}
					if (buf[i] != '\r')
					{
						if (buf[i] == '\n')
						{
							if (st->status == HIST_COMMAND)
							{
								st->histindex = 0;
								put_history(st);
								if (strlen(st->linebuf) > 0)
								{
									shift_history(st);
								}
							}
							st->bufindex = strlength(st->linebuf);
							if ((rv = hist_sendcmd(st)) != 0)
							{
								break;
							}
							st->bufindex = st->edited = st->histindex = 0;
							st->linebuf[(st->bufindex)] = 0;
						}
						else
						{
							char* x;
							x = st->linebuf + st->bufindex;
							memmove(x + 1, x, strlen(x) + 1);
							st->linebuf[(st->bufindex)++] = buf[i];
						}
					}
				}
			}
			st->lastchar = buf[i];
		}
	}
	return rv;
}

struct vdehiststat* vdehist_new(int termfd, SOCKET mgmtfd)
{
	struct vdehiststat* st;
	int i;
	if (commandlist == NULL && mgmtfd >= 0)
	{
		vdehist_create_commandlist(mgmtfd);
	}
	st = malloc(sizeof(struct vdehiststat));
	if (st)
	{
		if (mgmtfd < 0)
		{
			st->status = HIST_NOCMD;
		}
		else
		{
			st->status = HIST_COMMAND;
		}
		st->echo = 1;
		st->telnetprotocol = 0;
		st->bufindex = st->edited = st->histindex = st->vbufindex = st->vindata = st->lastchar = 0;
		st->linebuf[(st->bufindex)] = 0;
		st->vlinebuf[(st->vbufindex)] = 0;
		st->termfd = termfd;
		st->mgmtfd = mgmtfd;
		for (i = 0; i < HISTORYSIZE; i++)
		{
			st->history[i] = 0;
		}
	}
	return st;
}

void vdehist_free(struct vdehiststat* st)
{
	int index;
	if (st)
	{
		for (index = 0; index < HISTORYSIZE; index++)
		{
			if (st->history[index])
			{
				free(st->history[index]);
			}
		}
		free(st);
	}
}
int vdehist_getstatus(struct vdehiststat* st)
{
	return st->status;
}

void vdehist_setstatus(struct vdehiststat* st, int status)
{
	st->status = status;
}


int vdehist_gettermfd(struct vdehiststat* st)
{
	return st->termfd;
}


SOCKET vdehist_getmgmtfd(struct vdehiststat* st)
{
	return st->mgmtfd;
}

void vdehist_setmgmtfd(struct vdehiststat* st, SOCKET mgmtfd)
{
	st->mgmtfd = mgmtfd;
}

int strlength(const char* str)
{
	int length = 0;
	if (!str)
	{
		return 0;
	}
	while (*str!='\0')
	{
		str++;
		length++;
	}
	return length;
}