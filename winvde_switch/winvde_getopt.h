#pragma once
#include <string.h>
#include <stdio.h>

extern int opterr; //= 1,             /* if error message should be printed */
extern int optind;// = 1; // ,             /* index into parent argv vector */
extern int optopt;                 /* character checked for validity */
extern int optreset;               /* reset getopt */
extern char* optarg;                /* argument associated with option */

struct option {
	const char* name;
	int has_arg;
	int * flag;
	int val;
};

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

int getopt(int nargc, const char**nargv, const char* ostr);
int getopt_long(const int nargc, const char**nargv, const char* ostr, struct option* long_options, int* index);