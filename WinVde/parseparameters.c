
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <libwinvde.h>
#include <winvdeplugmodule.h>



// vde_grnam2gid not needed in windows

#define CHAR 0
#define QUOTE 1
#define DOUBLEQ 2
#define ESC 3
#define PERC 4
#define PER2 5
#define DELIM 6
#define END 7
#define TSIZE (END+1)

#define COPY 1
#define CPER 3

static char nextstate[TSIZE - 1][TSIZE] = {
	{CHAR,		QUOTE,	DOUBLEQ,ESC,	PERC,	0,	DELIM,	END},
	{QUOTE,		CHAR,	QUOTE,	QUOTE,	QUOTE,	0,	QUOTE,	END},
	{DOUBLEQ,	DOUBLEQ,CHAR,	DOUBLEQ,DOUBLEQ,0,	DOUBLEQ,END},
	{CHAR,		CHAR,	CHAR,	CHAR,	CHAR,	0,	CHAR,	END},
	{PER2,		QUOTE,	DOUBLEQ,ESC,	PERC,	0,	DELIM,	END},
	{CHAR,		QUOTE,	DOUBLEQ,ESC,	PERC,	0,	DELIM,	END},
	{CHAR,		QUOTE,	DOUBLEQ,ESC,	PERC,	0,	DELIM,	END},
};

static char action[TSIZE - 2][TSIZE] = {
	{COPY,	0,		0,		0,		COPY,	0,	0,		0}, // char
	{COPY,	0,		COPY,	COPY,	COPY,	0,	COPY,	0}, // quote
	{COPY,	COPY,	0,		COPY,	COPY,	0,	COPY,	0}, // dobuleq
	{COPY,	COPY,	COPY,	COPY,	COPY,	0,	COPY,	0}, // esc
	{COPY,	0,		0,		0,		COPY,	0,	0,		0}, // perc
	{COPY,	0,		0,		0,		COPY,	0,	0,		0}, // per2
};

static const char* hexchars = "0123456789ABCDEF0123456789abcdef";

uint32_t winvde_isnumber(const char* string)
{
	uint32_t value = 0;
	uint32_t last_value = 0;
	if (string != NULL)
	{
		while (*string != 0)
		{
			if (!isdigit(*string))
			{
				return 0;
			}
			if (value > 0 && last_value > 0 && value > UINT_MAX / 10)
			{
				return 0;
			}
			value = value * 10;
			if (value > 0 && last_value > 0 && value > UINT_MAX - (*string - 0x30))
			{
				return 0;
			}
			value +=  (*string - 0x30);
			if(last_value >value)
			{
				// wrap around
				return 0;
			}
			last_value = value;
			string++;
		}
	}
	return 1;
}

uint64_t winvde_string_to_ullm(const char* numstr)
{
	unsigned long long rval = 0xFFFFFFFFFFFFFFFF;
	char* tail = NULL;
	if (numstr != NULL)
	{
		rval = strtoull(numstr, &tail, 0);
		for (; *tail; tail++)
		{
			switch (*tail)
			{
			case  'k':
			case 'K':
				if (rval >= ((UINT64_MAX / rval) /(uint64_t)(1ULL << 20)))
				{
					// overflow
					errno = EINVAL;
					return 0xFFFFFFFFFFFFFFFF;
				}
				rval *= 1ULL << 10;
				break;
			case 'm':
			case 'M':
				if (rval >= ((UINT64_MAX / rval) / (uint64_t)(1ULL << 20)))
				{
					// overflow
					errno = EINVAL;
					return 0xFFFFFFFFFFFFFFFF;
				}
				rval *= 1ULL << 20;
				break;
			case 'g':
			case 'G':
				if (rval >= ((UINT64_MAX /rval) / (uint64_t)(1ULL << 30)))
				{
					// overflow
					errno = EINVAL;
					return 0xFFFFFFFFFFFFFFFF;
				}
				rval *= 1ULL << 30;
				break;
			case 't':
			case 'T':
				if (rval >= ((UINT64_MAX /rval) /(uint64_t)(1ULL << 40)))
				{
					// overflow
					errno = EINVAL;
					return 0xFFFFFFFFFFFFFFFF;
				}
				rval *= (uint64_t)1ULL << 40;
				break;
			case 'p':
			case 'P':
				if (rval >= ((UINT64_MAX / rval )/ (uint64_t)(1ULL << 50)))
				{
					// overflow
					errno = EINVAL;
					return 0xFFFFFFFFFFFFFFFF;
				}
				rval *= (uint64_t)(1ULL << 50);
				break;
			case 'e':
			case 'E':
				if (rval >= ((UINT64_MAX  / rval)/ (uint64_t)(1ULL << 60)))
				{
					// overflow
					errno = EINVAL;
					return 0xFFFFFFFFFFFFFFFF;
				}
				rval *= (uint64_t)(1ULL << 60);
				break;
			}
		}
	}
	else
	{
		errno = EINVAL;
	}
	return rval;
}

int winvde_ch2n(char x)
{
	char* n = strchr(hexchars, x);
	return n ? (n - hexchars) % 16L : - 1;
}

int winvde_token(char c, const char* delim)
{
	int this;
	if (!delim)
	{
		errno = EINVAL;
		return -1;
	}
	switch (c)
	{
	case 0:
		this = END;
		break;
	case '\'':
		this = QUOTE;
		break;
	case '\"':
		this = DOUBLEQ;
		break;
	case '\\':
		this = ESC;
		break;
	case '%':
		this = PERC;
		break;
	default:
		this = strchr(delim, c) == NULL ? CHAR : DELIM;
		break;

	}
	return this;
}

 char* strtokq_r(char* string, const char* delimiter, char** saveptr)
{
	char* begin;
	char* from;
	char* to;
	int status = CHAR;
	if (delimiter == NULL||saveptr==NULL){ return NULL; }
	begin = from = to = (string == NULL) ? *saveptr : string;
	if (from == NULL)
	{
		return NULL;
	}
	while (status != DELIM && status != END)
	{
		int this = winvde_token(*from, delimiter);
		int todo = action[status][this];
		if (todo & COPY)
		{
			if (to != from)
			{
				*to = *from;
			}
			if (todo == CPER)
			{
				char* perc = to - 2;
				int hex1 = winvde_ch2n(perc[1]);
				int hex2 = winvde_ch2n(perc[2]);
				if (hex1 >= 0 && hex2 >= 0)
				{
					*perc = hex1 * 0x10 + hex2;
					to = perc;
				}
			}
			to++;
		}
		from++;
		status = nextstate[status][this];
	}
	*to = 0;
	*saveptr = (status == END) ? NULL : from;
	return begin;
}

char* strtokq_nostrip_r(char* string, const char* delimiter, char** saveptr)
{
	char* begin;
	char* from;
	char* to;
	int status = CHAR;
	if (!delimiter || !saveptr)
	{
		return NULL;
	}
	begin = from = to = (string == NULL) ? *saveptr : string;
	if (from == NULL)
	{
		return NULL;
	}
	while (status != DELIM && status != END)
	{
		int this = winvde_token(*from, delimiter);
		if (this != DELIM)
		{
			to++;
		}
		from++;
		status = nextstate[status][this];
	}
	*to = 0;
	*saveptr = (status == END) ? NULL : from;
	return begin;
}

char* strtokq_rev_r(char* string, const char* delimiter)
{
	char* begin;
	char* scan;
	char* to;
	int status = CHAR;
	if (!string || !delimiter)
	{
		return NULL;
	}
	scan = begin = string;
	to = NULL;
	if(scan== NULL)
	{
		return NULL;
	}
	for (; status != END; scan++)
	{
		int this = winvde_token(*scan, delimiter);
		status = nextstate[status][this];
		if (status == DELIM)
		{
			to = scan;
		}
	}
	if (to != NULL)
	{
		*to = 0;
	}
	return begin;
}

int winvde_parseparameters(char* string, struct winvde_parameters* parameters)
{
	if (!string||!parameters)
	{
		return -1;
	}
	if (*string != 0)
	{
		char* sp;
		char* elem;
		elem = strtokq_r(string, "/", &sp);
		while ((elem = strtokq_r(NULL,"/", &sp)) != NULL)
		{
			char* eq = strchr(elem, '=');
			size_t taglen = eq ? eq - elem : strlen(elem);
			if (taglen > 0)
			{
				struct winvde_parameters* scan;
				for (scan = parameters; scan->tag; scan++)
				{
					if (strncmp(elem, scan->tag, taglen) == 0)
					{
						*(scan->value) = eq ? eq + 1 : "";
						break;
					}
				}
				if (scan->tag == NULL)
				{
					fprintf(stderr, "unknown key: %*.*s\n", (int)taglen, (int)taglen, elem);
					errno = EINVAL;
					return -1;
				}
			}
		}

	}
	return 0;
}

int winvde_parsepathparameters(char* string, struct winvde_parameters* parameters)
{
	char* sp = NULL;
	char* elem = NULL;
	char* bracket=NULL;
	if (!string || !parameters)
	{
		return -1;
	}
	if (*string != 0)
	{

		elem = strtokq_r(string, "[", &sp);
		for (bracket = strtokq_rev_r(sp, "]");
			(elem = strtokq_r(bracket, "/", &sp)) != NULL; bracket = NULL)
		{
			char* eq = strchr(elem, '=');
			size_t taglen = eq ? eq - elem : strlen(elem);
			if (taglen > 0)
			{
				struct winvde_parameters* scan;
				for (scan = parameters; scan->tag; scan++)
				{
					if (strncmp(elem, scan->tag, taglen) == 0)
					{
						*(scan->value) = eq ? eq + 1:"";
						break;
					}
				}
				if (scan->tag == NULL)
				{
					if (eq == NULL && parameters->tag != NULL && parameters->tag[0] == 0)
					{
						*(parameters->value) = elem;
					}
					else
					{
						fprintf(stderr, "unknown key: %*.*s\n", (int)taglen, (int)taglen, elem);
						errno = EINVAL;
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

char* winvde_parsenestparameters(char * string)
{
	if (!string)
	{
		return NULL;
	}
	if (*string != 0)
	{
		char* sp;
		char* bracket;
		strtokq_nostrip_r(string, "{", &sp);
		bracket = strtokq_rev_r(sp, "}");
		return bracket;
	}
	return NULL;
}

