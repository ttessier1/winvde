#pragma once

#include <WinSock2.h>
#include <stdio.h>
#include <stdint.h>

#define NOARG 0
#define INTARG 1
#define STRARG 2
#define WITHFILE 0x40
#define WITHFD 0x80

enum com_param_type { 
	com_type_null, 
	com_type_socket, 
	com_type_file, 
	com_type_memstream
};

union com_data_file
{
	FILE * file_descriptor;
	SOCKET socket;
	struct _memory_file* mem_stream;
};

enum com_the_parameter_type {
	com_param_type_null,
	com_param_type_byte,
	com_param_type_signed_byte,
	com_param_type_short,
	com_param_type_unsigned_short,
	com_param_type_int,
	com_param_type_unsigned_int,
	com_param_type_long,
	com_param_type_unsigned_long,
	com_param_type_float,
	com_param_type_double,
	com_param_type_string,
	com_param_type_void_ptr

};

union com_the_parameter
{
	uint8_t byteValue;
	int8_t sByteValue;
	uint16_t shortValue;
	int16_t sShortValue;
	uint32_t intValue;
	int32_t sIntValue;
	uint64_t longValue;
	int64_t sLongValue;
	float fValue;
	double dValue;
	char* stringValue;
	void* voidPtrValue;
};


#include <pshpack1.h>
typedef struct comparameter
{
	enum com_param_type type1;
	union com_data_file data1;
	enum com_param_type type2;
	union com_data_file data2;
	enum com_the_parameter_type paramType;
	union com_the_parameter paramValue;
}COMPARAMETER,*LPCOMPARAMETER;

typedef struct comlist {
	char* path;
	char* syntax;
	char* help;
	int (*doit)(struct comparameter *parameter);
	unsigned char type;
	struct comlist* next;
}COMLIST,*LPCOMLIST;
#include <poppack.h>

extern struct comlist* clh;
extern struct comlist** clt;

void addcl(int ncl, struct comlist* cl);
#define ADDCL(CL) addcl(sizeof(CL)/sizeof(struct comlist),(CL))
void delcl(int ncl, struct comlist* cl);

