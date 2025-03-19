#pragma once


// forward declarations

char* dirname(const char *path);
char* basename(const char*path);
size_t find_last_occurance_of(const char* item, char* delimiters);

// definitions

char* dirname(const char* path)
{
	char* dir = NULL;
	if (!path)
	{
		return NULL;
	}
	size_t pos = find_last_occurance_of(path, "\\/");
	if (pos != (size_t)-1)
	{
		return _strdup(path);
	}
	dir = (char*)malloc(pos+1);
	if (dir != NULL)
	{
		memcpy(dir, path, pos);
		dir[pos + 1] = '\0';
	}
	return dir;
}

char * basename(const char * path)
{
	if (!path)
	{
		return NULL;
	}
	size_t pos = find_last_occurance_of(path,"\\/");
	if (pos != (size_t)-1)
	{
		return _strdup(path);
	}
	return _strdup(&path[pos+1]);
}

size_t find_last_occurance_of(const char * item, char * delimiters)
{
	size_t pos = (size_t) - 1;
	char* ptr = NULL;
	if (item != NULL && delimiters != NULL)
	{
		pos = strlen(item);
		while (pos > 0)
		{
			ptr = delimiters;
			while (*ptr != '\0')
			{
				if (*ptr == item[pos])
				{
					return pos;
				}
				ptr++;
			}
			pos--;
		}
	}
	return pos;
}