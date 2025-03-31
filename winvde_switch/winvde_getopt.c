#include "winvde_getopt.h"
#include "winvde_mgmt.h"

int opterr = 1 ;
int optind = 1 ;
int optopt = 0 ;
int optreset = 0 ;
char* optarg = NULL ;

int getopt(int nargc, const char* *nargv, const char* ostr)
{
    char* argument_place = (char*)EMSG;
    char* option_letter_index = NULL;
    if (nargc <= 0 || nargv == NULL || ostr == NULL)
    {
        switch_errno = EINVAL;
        return -1;
    }
    if (optreset==1)
    {
        optreset = 0; 
        // set argument_place and check optind
        if (
            optind >= nargc || 
            *(argument_place = (char*)nargv[optind]) != '-')
        {
            argument_place = EMSG;
            switch_errno = EINVAL;
            return -1;
        }
        if (argument_place[1] && *--argument_place == '-') // did we find --
        {
            ++optind;
            argument_place = EMSG;
            return -1;
        }
    }
    // set option to whatever the argument_place pointer is
    if (
        (optopt = (int)*argument_place++) == (int)':'||
        !(option_letter_index = strchr(ostr,optopt))
        )
    {
        if (optopt == (int)'-')
        {
            return -1;
        }
        if (!*argument_place)
        {
            ++optind;
        }
        if (opterr && *ostr != ':')
        {
            fprintf(stderr, "illegal option -- %c\n",optopt);
        }
        return (BADCH);
    }
    if (*++option_letter_index != ':')
    {
        optarg = NULL;
        if (!*argument_place)
        {
            ++optind;
        }
    }
    else
    {
        if (*argument_place)
        {
            optarg = argument_place;
        }
        else if ( nargc <= ++optind)
        {
            argument_place = EMSG;
            if (*ostr == ':')
            {
                return(BADARG);
            }
            if (opterr)
            {
                fprintf(stderr, "Option requires an argument -- %c\n",optopt);
            }
            return (BADARG);
        }
        else
        {
            optarg = (char*)nargv[optind];
        }
        argument_place = EMSG;
        ++optind;
    }
    return (optopt);
}

int getopt_long(const int nargc, const char **nargv, const char* ostr, struct option* long_options, int* index)
{
    char* argument_place = (char*)EMSG;
    char* option_letter_index = NULL;
    struct option* ptr = NULL;
    if (nargc <= 0 || nargv == NULL || ostr == NULL || long_options == NULL || index==NULL)
    {
        switch_errno = EINVAL;
        return -1;
    }
    if (optreset == 1||!*argument_place)
    {
        optreset = 0;
        // set argument_place and check optind
        if (
            optind >= nargc ||
            *(argument_place = (char*)nargv[optind]) != '-')
        {
            argument_place = EMSG;
            switch_errno = EINVAL;
            return -1;
        }
        if (argument_place[1] && *++argument_place == '-') // did we find --
        {
            argument_place++;
            ptr = long_options;
            while (ptr && ptr->name)
            {
                if (strcmp(ptr->name, argument_place) == 0)
                {
                    optopt = ptr->val;
                    if (ptr->has_arg)
                    {
                        optind++;
                        optarg = (char*)nargv[optind];
                    }
                    optind++;
                    argument_place = EMSG;
                    return optopt;
                }
                ptr++;
            }
            fprintf(stderr, "illegal option -- %s\n", argument_place);
            argument_place = EMSG;
            switch_errno = EINVAL;
            return -1;
        }
    }
    // set option to whatever the argument_place pointer is
    if (
        (optopt = (int)*argument_place++) == (int)':' ||
        !(option_letter_index = strchr(ostr, optopt))
        )
    {
        if (optopt == (int)'-')
        {
            return -1;
        }
        if (!*argument_place)
        {
            ++optind;
        }
        if (opterr && *ostr != ':')
        {
            fprintf(stderr, "illegal option -- %c\n", optopt);
        }
        return (BADCH);
    }
    if (*++option_letter_index != ':')
    {
        optarg = NULL;
        if (!*argument_place)
        {
            ++optind;
        }
    }
    else
    {
        if (*argument_place)
        {
            optarg = argument_place;
        }
        else if (nargc <= ++optind)
        {
            argument_place = EMSG;
            if (*ostr == ':')
            {
                return(BADARG);
            }
            if (opterr)
            {
                fprintf(stderr, "Option requires an argument -- %c\n", optopt);
            }
            return (BADARG);
        }
        else
        {
            optarg = (char*)nargv[optind];
        }
        argument_place = EMSG;
        ++optind;
    }
    return (optopt);
}