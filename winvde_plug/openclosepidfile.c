#include "openclosepidfile.h"
#include <stdio.h>
#include <stdlib.h>

void openclosepidfile(char* pidfile) {
	static char* savepidfile= NULL;
	if (pidfile != NULL)
	{
		FILE* f = fopen(pidfile, "w");
		if (f != NULL) {
			savepidfile = pidfile;
			fprintf(f, "%d\n", getpid());
			fclose(f);
		}
	}
	else if (savepidfile != NULL)
	{
		_unlink(savepidfile);
	}
}