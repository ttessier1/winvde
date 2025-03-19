// TestParser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libwinvde.h>
#include <errno.h>
#include <winvdeplugmodule.h>

typedef uint32_t BOOL;

#define TRUE 1
#define FALSE 0

// Link with the static lib
#pragma comment(lib,"winvde.lib")

void GetOptions(const int argc, char ** argv);
void VerboseArguments(const int argc, const char ** argv);
BOOL ValidateArguments(struct winvde_parameters* parameters);

BOOL TestTok(const char * tokenString);
BOOL TestIsNumber(const char* numberString);
BOOL TestStrToLLM(const char* numberString);
BOOL TestC2Hn(const char* numberString);
BOOL TestToken(const char tokenChar, const char* delim);
BOOL TestStrTokQ_R(const char* strToQ_RString, const char* delimiter);
BOOL TestStrTokQ_nostrip_R(const char* strToQ_RString, const char* delimiter);
BOOL TestStrTokQ_rev_R(const char* strToQ_RString, const char* delimiter);

/*

Example Usage: testparser.exe --url winvde:/ttl=10/port=1234/vni=10
Example Usage: testparser.exe --url tcp:/ttl=10/port=1234/vni=10
Example Usage: testparser.exe --test-tok 123=&
Example Usage: testparser.exe --test-isnumber 12345
Example Usage: testparser.exe --test-strtoullm 123G
Example Usage: testparser.exe --test-ch2n 1234ABCD
Example Usage: testparser.exe --test-token c / /
Example Usage: testparser.exe --test-strtokq_r 
Example Usage: testparser.exe --test-strtokq_nostrip_r
Example Usage: testparser.exe --test-strtokq_rev_r
Example Usage: testparser.exe --test-winvde_parseparameters
Example Usage: testparser.exe --test-winvde_parsepathparameters 
Example Usage: testparser.exe --test-winvde_parsenestparameters { }
*/

BOOL DoTestTok = FALSE;
BOOL DoTestIsNumber = FALSE;
BOOL DoTestStrToLLM = FALSE;
BOOL DoTestCh2N = FALSE;
BOOL DoTestToken = FALSE;
BOOL DoTestStrTokQ_R = FALSE;
BOOL DoTestStrTokQ_nostrip_R = FALSE; 
BOOL DoTestStrTokQ_rev_R = FALSE;
BOOL DoTestWinVdeParseParameters = FALSE;
BOOL DoTestWinVdeParsePathParameters = FALSE;
BOOL DoTestWinVdeParseNestParameters = FALSE;

char * TokString = NULL;
char * NumberString = NULL;
char * StrToLLMString = NULL;
char * Ch2NString = NULL;
char * TokenString = NULL;
char * TokenDelimiter = NULL;
char * StrTokQ_RString = NULL;
char * StrTokQ_RDelimiter = NULL;
char * StrTokQ_nostrip_RString = NULL;
char * StrTokQ_nostrip_RDelimiter = NULL;
char * StrTokQ_rev_RString = NULL;
char * StrTokQ_rev_RDelimiter = NULL;
char * WinVdeParseParametersString = NULL;
char * WinVdeParsePathParametersString = NULL;
char * WinVdeParseNestParametersString = NULL;

int main(const int argc, const char** argv)
{
	const char* portstr = "12345";
	const char* vnistr = "1";
	const char* ttlstr = "1";
	struct winvde_parameters parameters[4] = {
		{(char*)"port", (char**)&portstr},
		{(char*)"vni", (char**)&vnistr},
		{(char*)"ttl", (char**)&ttlstr},
		{NULL, NULL}
	};

	if (argc == 0 || argv == NULL)
	{
		fprintf(stderr, "Invalid Exe\n");
		exit(0xFFFFFFFF);
	}

	if (argc < 2)
	{
		fprintf(stderr, "Invalid arguments\n");
		exit(0xFFFFFFFE);
	}

	VerboseArguments(argc, argv);

	GetOptions(argc, argv);

	if (DoTestTok)
	{
		if (!TestTok(TokString))
		{
			fprintf(stderr, "Failed TestTok: [%s]\n",TokString);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestIsNumber)
	{
		if (!TestIsNumber(NumberString))
		{
			fprintf(stderr, "Failed TestIsNumber: [%s]\n", NumberString);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestStrToLLM)
	{
		if (!TestStrToLLM(StrToLLMString))
		{
			fprintf(stderr, "Failed TestStrToLLM: %s\n", StrToLLMString);
			exit(0xFFFFFFFF);
		}
	}
	else if(DoTestCh2N)
	{
		if (!TestC2Hn(Ch2NString))
		{
			fprintf(stderr, "Failed TestC2Hn: %s\n", Ch2NString);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestToken)
	{
		if (!TestToken(TokenString[0], TokenDelimiter))
		{
			fprintf(stderr, "Failed TestToken: %c %s\n", TokenString[0], TokenDelimiter);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestStrTokQ_R)
	{
		if (!TestStrTokQ_R(StrTokQ_RString, StrTokQ_RDelimiter))
		{
			fprintf(stderr, "Failed TestStrTokQ_R: %s %s\n", StrTokQ_RString, StrTokQ_RDelimiter);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestStrTokQ_nostrip_R)
	{
		if(!TestStrTokQ_nostrip_R(StrTokQ_nostrip_RString,StrTokQ_nostrip_RDelimiter))
		{
			fprintf(stderr, "Failed TestStrTokQ_nostrip_R: %s %s\n", StrTokQ_RString, StrTokQ_nostrip_RDelimiter);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestStrTokQ_rev_R)
	{
		if (!TestStrTokQ_rev_R(StrTokQ_rev_RString, StrTokQ_rev_RDelimiter))
		{
			fprintf(stderr, "Failed TestStrTokQ_rev_R: %s %s\n", StrTokQ_rev_RString, StrTokQ_rev_RDelimiter);
			exit(0xFFFFFFFF);
		}
	}
	else if (DoTestWinVdeParseParameters)
	{
		printf("%s\n\n", WinVdeParseParametersString);

		winvde_parseparameters(WinVdeParseParametersString, parameters);

		if (ValidateArguments(parameters))
		{

			printf("Type %s\n", WinVdeParseParametersString);

			struct winvde_parameters* scan;
			for (scan = parameters; scan->tag; scan++)
			{

				printf("%s %s\n", scan->tag == NULL ? "(NULL)" : scan->tag, scan->value == NULL ? "(INVALID)" : *(scan->value) == NULL ? "(NULL)" : *(scan->value));
			}
		}
		else
		{
			fprintf(stderr, "Failed to validate the parameters\n");
			exit(0xFFFFFFFF);
		}

	}
	else if (DoTestWinVdeParsePathParameters)
	{
		if (winvde_parsepathparameters(WinVdeParsePathParametersString, parameters) != 0)
		{
			fprintf(stderr, "Failed to DoTestWinVdeParsePathParameters: %s\n", WinVdeParsePathParametersString);
			exit(0xFFFFFFFF);
		}
		else
		{
			struct winvde_parameters* scan;
			for (scan = parameters; scan->tag; scan++)
			{
				printf("%s %s\n", scan->tag == NULL ? "(NULL)" : scan->tag, scan->value == NULL ? "(INVALID)" : *(scan->value) == NULL ? "(NULL)" : *(scan->value));
			}
		}
		
	}
	else if (DoTestWinVdeParseNestParameters)
	{
		fprintf(stderr, "DoTestWinVdeParseNestParameters Not Implemented\n");
		exit(0xFFFFFFFF);
	}
	else
	{
		fprintf(stderr,"Invalid Parameters\n");
	}
}

void VerboseArguments(const int argc, const char** argv)
{
	uint32_t index = 0;
	if (argc <= 0 || argv == NULL)
	{
		return;
	}
	for (index = 0; index < (uint32_t)argc; index++)
	{
		if (argv[index] != NULL)
		{
			fprintf(stdout, "--argument: %d %s\n", index, argv[index]);
		}
		else
		{
			fprintf(stdout, "--argument: %d %s\n", index, "(NULL)");
		}
	}
}

/*

*/
void GetOptions(const int argc, char ** argv)
{
	uint32_t index = 0;
	char* parameter = NULL;
	char* parameter2 = NULL;
	if (argv == NULL || argc <= 1) { return; }
	for (index = 1; index < (uint32_t)argc; index++)
	{
		parameter = NULL;
		if (argv[index] == NULL)
		{
			return;
		}
		if (index < (uint32_t)argc - 1)
		{
			parameter = argv[index + 1];
		}
		if (index < (uint32_t)argc - 2)
		{
			parameter2 = argv[index + 2];
		}
		if (strcmp(argv[index], "--test-tok") == 0)
		{
			if (!DoTestTok)
			{
				DoTestTok = TRUE;
				if (parameter != NULL)
				{
					TokString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "TestTok requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestTok can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else if (strcmp(argv[index], "--test-isnumber") == 0)
		{
			if (!DoTestIsNumber)
			{
				DoTestIsNumber = TRUE;
				if (parameter != NULL)
				{
					NumberString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "TestIsNumber requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestIsNumber can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-strtoullm") == 0)
		{
			if (!DoTestStrToLLM)
			{
				DoTestStrToLLM = TRUE;
				if (parameter != NULL)
				{
					StrToLLMString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "Teststrtoullm requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "Teststrtoullm can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-ch2n") == 0)
		{
			if (!DoTestCh2N)
			{
				DoTestCh2N = TRUE;
				if (parameter != NULL)
				{
					Ch2NString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "TestCh2N requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestCh2N can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-token") == 0)
		{
			if (!DoTestToken)
			{
				DoTestToken = TRUE;
				if (parameter != NULL && parameter2 != NULL)
				{
					TokenString = parameter;
					TokenDelimiter = parameter2;
					index++;
					index++;
				}
				else
				{
					fprintf(stderr, "TestToken requires 2 parameters\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestToken can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-strtokq_r") == 0)
		{
			if (!DoTestStrTokQ_R)
			{
				DoTestStrTokQ_R = TRUE;
				if (parameter != NULL && parameter2 != NULL)
				{
					StrTokQ_RString = parameter;
					StrTokQ_RDelimiter = parameter2;
					index++;
					index++;
				}
				else
				{
					fprintf(stderr, "TestStrTokQ_R requires 2 parameters parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestStrTokQ_R can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-strtokq_nostrip_r") == 0)
		{
			if (!DoTestStrTokQ_nostrip_R)
			{
				DoTestStrTokQ_nostrip_R = TRUE;
				if (parameter != NULL && parameter2 != NULL)
				{
					StrTokQ_nostrip_RString = parameter;
					StrTokQ_nostrip_RDelimiter = parameter2;
					index++;
					index++;
				}
				else
				{
					fprintf(stderr, "TestStrTokQ_nostrip_R requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestStrTokQ_nostrip_R can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-strtokq_rev_r") == 0)
		{
			if (!DoTestStrTokQ_rev_R)
			{
				DoTestStrTokQ_rev_R = TRUE;
				if (parameter != NULL && parameter2 != NULL)
				{
					StrTokQ_rev_RString = parameter;
					StrTokQ_rev_RDelimiter = parameter2;
					index++;
					index++;
				}
				else
				{
					fprintf(stderr, "TestStrTokQ_rev_R requires 2 parameters\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestStrTokQ_rev_R can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-winvde_parseparameters") == 0)
		{
			if (!DoTestWinVdeParseParameters)
			{
				DoTestWinVdeParseParameters = TRUE;
				if (parameter != NULL)
				{
					WinVdeParseParametersString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "TestWinVdeParseParameters requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestWinVdeParseParameters can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-winvde_parsepathparameters") == 0)
		{
			if (!DoTestWinVdeParsePathParameters)
			{
				DoTestWinVdeParsePathParameters = TRUE;
				if (parameter != NULL)
				{
					WinVdeParsePathParametersString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "TestWinVdeParsePathParameters requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestWinVdeParsePathParameters can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else  if (strcmp(argv[index], "--test-winvde_parsenestparameters") == 0)
		{
			if (!DoTestWinVdeParseNestParameters)
			{
				DoTestWinVdeParseNestParameters = TRUE;
				if (parameter != NULL)
				{
					WinVdeParseNestParametersString = parameter;
					index++;
				}
				else
				{
					fprintf(stderr, "TestWinVdeParseNestParameters requires a parameter\n");
					exit(0xFFFFFFFF);
				}
			}
			else
			{
				fprintf(stderr, "TestWinVdeParseNestParameters can only be specified once\n");
				exit(0xFFFFFFFF);
			}
		}
		else
		{
			fprintf(stderr, "Invalid Argument: %s", argv[index]!=NULL?argv[index]:"(NULL)");
			exit(0xFFFFFFFF);
		}
	}

	
}

BOOL ValidateArguments(struct winvde_parameters* parameters)
{
	BOOL returnCode = FALSE;
	BOOL portSet = FALSE;
	BOOL vniSet = FALSE;
	BOOL ttlSet = FALSE;
	BOOL portVisible = FALSE;
	BOOL ttlVisible = FALSE;
	BOOL vniVisible = FALSE;
	uint32_t value = 0;
	if (parameters == NULL)
	{
		errno = EINVAL;
		return FALSE;
	}
	struct winvde_parameters* scan;
	returnCode = TRUE;
	for (scan = parameters; scan->tag && returnCode == TRUE; scan++)
	{
		if (strcmp(scan->tag, "port") == 0)
		{
			if (!portVisible)
			{

				portVisible = TRUE;
				// max is 48 min is 1 ( what switch port )
				if (scan->value != NULL && (*scan->value) != NULL)
				{
					errno = 0;
					value = atoi(*scan->value);
					if (errno != ERANGE && errno != EINVAL)
					{
						if (value >= 1 && value <= 65535
							)
						{

						}
						else
						{
							errno = EINVAL;
							returnCode = FALSE;
						}
					}
					else
					{
						errno = EINVAL;
						returnCode = FALSE;
					}
				}
				else
				{
					errno = EINVAL;
					returnCode = FALSE;
				}
			}
			else
			{
				errno = EINVAL;
				returnCode = FALSE;
			}
		}
		else if (strcmp(scan->tag, "vni") == 0)
		{
			if (!vniVisible)
			{
				vniVisible = TRUE;
				if (scan->value != NULL && (*scan->value) != NULL)
				{
					errno = 0;
					value = atoi(*scan->value);
					if (errno != ERANGE && errno != EINVAL)
					{
						if (value >= 1 && value <= 4096)
						{

						}
						else
						{
							errno = EINVAL;
							returnCode = FALSE;
						}
					}
					else
					{
						errno = EINVAL;
						returnCode = FALSE;
					}
				}
				else
				{
					errno = EINVAL;
					returnCode = FALSE;
				}
			}
			else
			{
				errno = EINVAL;
				returnCode = FALSE;
			}
			// max is 4096 min is 1 ( what is our virtual nework indicator )
		}
		else if (strcmp(scan->tag, "ttl") == 0)
		{
			// max is 255 min is 1 ( how many devices can we hit )
			if (!ttlVisible)
			{
				ttlVisible = TRUE;
				if (scan->value != NULL && (*scan->value) != NULL)
				{
					errno = 0;
					value = atoi(*scan->value);
					if (errno != ERANGE && errno != EINVAL)
					{
						if (value >= 1 && value <= 255)
						{

						}
						else
						{
							errno = EINVAL;
							returnCode = FALSE;
						}
					}
					else
					{
						errno = EINVAL;
						returnCode = FALSE;
					}
				}
				else
				{
					returnCode = FALSE;
					fprintf(stderr, "TTL Already Specified\n");
				}
			}
		}
	}
	return returnCode;
}

BOOL TestTok(const char* tokenString)
{
	fprintf(stderr, "TestTok Not Implemented");
	
	return FALSE;
}

BOOL TestIsNumber(const char* numberString)
{
	return winvde_isnumber(numberString)==1;
}

BOOL TestStrToLLM(const char* numberString)
{
	uint64_t retval = winvde_string_to_ullm(numberString);
	if (retval == 0xFFFFFFFFFFFFFFFF && errno == EINVAL)
	{
		return FALSE;
	}
	else
	{
		fprintf(stdout, "Number: %I64d %I64x\n", retval, retval);
		return TRUE;
	}
}

BOOL TestC2Hn(const char* numberString)
{
	int retval = winvde_ch2n(numberString[0]);
	if (retval == -1)
	{
		return FALSE;
	}
	else
	{
		fprintf(stdout, "Number:%d\n",retval);
		return TRUE;
	}

}

// pass char 'A' or 'a' and receive 6

// pass ' ' receive 1
// pass " " receive 2
// pass \ \ receive 3
// pass % % receive  4
// pass \0 \0 receive 7
// pass \0 \0 receive


BOOL TestToken(const char tokenChar, const char * delim)
{
	int retval = winvde_token(tokenChar, delim);
	if (retval == -1)
	{
		return FALSE;
	}
	else
	{
		fprintf(stdout, "Token Result: %d\n",retval);
	}
	return TRUE;
}

// 

BOOL TestStrTokQ_R(const char* strToQ_RString, const char * delimiter)
{
	char* saveptr = NULL;
	char * elem = strtokq_r((char*)strToQ_RString, delimiter, &saveptr);
	while ((elem = strtokq_r(NULL, "/", &saveptr)) != NULL)
	{
		char* eq = strchr(elem, '=');
		size_t taglen = eq ? eq - elem : strlen(elem);
		if (taglen > 0)
		{
			fprintf(stdout,"Tag:%*.*s Value: %s\n", (int)taglen, (int)taglen, elem, eq + 1);
		}
	}
	return TRUE;
}

BOOL TestStrTokQ_nostrip_R(const char* strToQ_RString, const char* delimiter)
{
	char* saveptr = NULL;
	char* elem = strtokq_nostrip_r((char*)strToQ_RString, delimiter, &saveptr);
	while ((elem = strtokq_nostrip_r(NULL, "/", &saveptr)) != NULL)
	{
		char* eq = strchr(elem, '=');
		size_t taglen = eq ? eq - elem : strlen(elem);
		if (taglen > 0)
		{
			fprintf(stdout, "Tag:%*.*s Value: %s\n", (int)taglen, (int)taglen, elem, eq + 1);
		}
	}
	return TRUE;
}

BOOL TestStrTokQ_rev_R(const char* strToQ_RString, const char* delimiter)
{
	char* saveptr = NULL;
	char* elem = strtokq_rev_r((char*)strToQ_RString, delimiter);
	if (elem != NULL)
	{
		fprintf(stdout, "Return: %s\n", elem);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}