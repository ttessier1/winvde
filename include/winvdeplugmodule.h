#pragma once

#define MAXDESCR 128
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <libwinvde.h>

#include <time.h>
#include <stdint.h>

struct windevconn;

#include <pshpack1.h>

struct winvdeplug_module
{
	int flags;
	WINVDECONN* (*winvde_open_real)(const char* given_winvde_url, const char* description, int interface_version, struct winvde_open_args* open_args);
	size_t(*winvde_recv)(WINVDECONN* conn, const char* buff, size_t length, int flags);
	size_t(*winvde_send)(WINVDECONN* conn, const char* buff, size_t length, int flags);
	int (*winvde_datafiledescriptor)(WINVDECONN* conn);
	int (*winvde_ctlfiledescriptor)(WINVDECONN* conn);
	int (*winvde_close)(WINVDECONN* conn);
};

struct winvdeconn {
	HANDLE handle;
	BOOL opened;
	struct winvdeplug_module module;
	unsigned char data[1];
};

struct winvde_parameters
{
	char* tag;
	char** value;
};

#include <poppack.h>

int winvde_parseparameters(char* str, struct winvde_parameters* parameters);
int winvde_parsepathparameters(char* str, struct winvde_parameters* parameters);
char* winvde_parsenestparameters(char* str);

struct winvde_hash_table;

void* winvde_find_in_hash(struct winvde_hash_table* table, unsigned char* destination, int vlan, time_t too_old);
void winvde_find_in_hash_update(struct winvde_hash_table* table, unsigned char* source, int vlan, void* payload, time_t now);
void winvde_hash_delete(struct winvde_hash_table* table, void* payload);
struct winvde_hash_table* _winvde_hash_table_init(size_t payload_size, uint32_t hashsize, uint64_t seed);
#define winvde_hash_init(type,hashsize,seed) _winvde_hash_table_init(sizeof(type),(hashsize),(seed));

void winvde_hash_table_finalize(struct winvde_hash_table * table);


#if defined(TEST)

uint32_t winvde_isnumber(const char* string);
uint64_t winvde_string_to_ullm(const char* numstr);
int winvde_ch2n(char x);
int winvde_token(char c, const char* delim);
char* strtokq_r(char* string, const char* delimiter, char** saveptr);
char* strtokq_nostrip_r(char* string, const char* delimiter, char** saveptr);
char* strtokq_rev_r(char* string, const char* delimiter);

#endif