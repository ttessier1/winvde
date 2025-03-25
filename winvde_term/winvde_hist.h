#pragma once

#include <WinSock2.h>

#define BUFSIZE 1024
#define HISTORYSIZE 32
#define CC_HEADER 0
#define CC_BODY 1
#define CC_TERM 2


extern char* prompt;

static char** commandlist;

typedef int(*int_socket_function)(SOCKET, char*, int, int );
typedef int(*int_file_function)(int,void *, unsigned int );


#define HIST_COMMAND 0x0
#define HIST_NOCMD 0x1
#define HIST_PASSWDFLAG 0x80

struct vdehiststat {
	unsigned char status;
	unsigned char echo;
	unsigned char telnetprotocol;
	unsigned char edited; /* the linebuf has been modified (left/right arrow)*/
	unsigned char vindata; /* 1 when in_data... (0000 end with .)*/
	char lastchar; /* for double tag*/
	char linebuf[BUFSIZE]; /*line buffer from the user*/
	int  bufindex; /*current editing position on the buf */
	char vlinebuf[BUFSIZE + 1]; /*line buffer from vde*/
	int  vbufindex; /*current editing position on the buf */
	char* history[HISTORYSIZE]; /*history of the previous commands*/
	int histindex; /* index on the history (changed with up/down arrows) */
	int termfd; /* fd to the terminal */
	SOCKET mgmtfd; /* mgmt fd to vde_switch */
};

struct vh_readln {
	int readbufsize;
	int readbufindex;
	char readbuf[BUFSIZE];
};

char* nologin(char* cmd, int len, struct vdehiststat* st);
int commonprefix(char* x, char* y, int maxlen);
void showexpand(char* linebuf, int bufindex, int termfd);
int tabexpand(char* linebuf, int bufindex, int maxlength);
int qstrcmp(const void* a, const void* b);
char* vdehist_readln(SOCKET vdefd, char* linebuf, int size, struct vh_readln* vlb);
void vdehist_create_commandlist(SOCKET vdefd);
void erase_line(struct vdehiststat* st, int prompt_too);
void redraw_line(struct vdehiststat* st, int prompt_too);
void vdehist_mgmt_to_term(struct vdehiststat* st);
int hist_sendcmd(struct vdehiststat* st);
void put_history(struct vdehiststat* st);
void get_history(int change, struct vdehiststat* st);
void shift_history(struct vdehiststat* st);
void telnet_option_send3(int fd, int action, int object);
int telnet_options(struct vdehiststat* st, unsigned char* s);
int vdehist_term_to_mgmt(struct vdehiststat* st);
struct vdehiststat* vdehist_new(int termfd, SOCKET mgmtfd);
void vdehist_free(struct vdehiststat* st);
int vdehist_getstatus(struct vdehiststat* st);
void vdehist_setstatus(struct vdehiststat* st, int status);
int vdehist_gettermfd(struct vdehiststat* st);
SOCKET vdehist_getmgmtfd(struct vdehiststat* st);
void vdehist_setmgmtfd(struct vdehiststat* st, SOCKET mgmtfd);
int strlength(const char* str);
