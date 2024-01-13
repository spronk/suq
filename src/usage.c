/* This source code is part of

suq, the Single-User Queuer

Copyright (c) 2010-2024 Sander Pronk
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>


#include "usage.h"
#include "log_err.h"

const char *usage_string =
"Usage: suq run [-d workdir] [-n ntasks] [-p pri] cmd args\n"
"       suq del [all|id]\n"
"       suq pri id priority\n"
"       suq ls\n"
"       suq wait [all|id]\n"
"       suq ntask n\n"
"       suq help\n"
"\n"
"suq, the Single User Queuer, takes shell commands and queues them to run in\n"
"parallel.\n"
"\n"
"Command summary:\n"
"\n"
"suq run [-d workdir] [-n ntasks] [-p pri] cmd args\n"
"   Submits a job for running. This job has command cmd and (optional)\n"
"   arguments.\n"
"\n"
"suq del [id|all]\n"
"   Deletes a job from the queue, and kills the job if it is already running.\n"
"   If 'all' is specified, all jobs are deleted\n"
"\n"
"suq pri id priority\n"
"   Sets a new priority p to job id 'id'. For id=='all', all jobs are\n"
"   re-prioritized.\n"
"\n"
"suq ls\n"
"   Lists all jobs in the queue\n"
"\n"
"suq info id\n"
"   Gives detailed information about a job.\n"
"\n"
"suq wait [all|id]\n"
"   Wait until jobs complete. If the argument is:\n"
"     (empty) : Wait until all jobs submitted at the time of the wait command \n"
"               have completed.\n"
"     'all'   : Wait until all jobs are finished\n"
"     id      : Wait until a specific job with given id is finished\n"
"\n"
"suq ntask n\n"
"   Sets the total number of tasks (processes/threads) that may run \n"
"   simultaneously.\n"
"\n"
"suq help\n"
"   Prints a more complete help message\n";


static char* find_manpage(const char *argvzero)
{
    char *manpage_dir;
    char *last;
    char *installdir=NULL;
    const char* dir_tries_pre[] = { ".",
                                    "..",
                                    "../..",
                                    "share",
                                    "../share",
                                    "../../share",
                                    NULL };
    const char* dir_tries_post[] = { "man", "man/man1" , NULL };
    int i,j;


    manpage_dir=malloc_check(PATH_MAX*sizeof(char));
    /* first try to see whether there's a path in argvzero */
    if ( (last=strrchr(argvzero, '/')) )
    {
        char *compose_str=malloc_check(PATH_MAX*sizeof(char));

        strncpy(compose_str, argvzero, last-argvzero);
        compose_str[last-argvzero] = 0;

        i=0;
        /* now try a few variants */
        while(dir_tries_pre[i] != NULL)
        {
            j=0;
            while(dir_tries_post[j] != NULL)
            {
                snprintf(manpage_dir, PATH_MAX,
                         "%s/%s/%s/suq.1", compose_str,
                                           dir_tries_pre[i],
                                           dir_tries_post[j]);
                if (access(manpage_dir, R_OK) == 0)
                {
                    free(compose_str);
                    return manpage_dir;
                }
                j++;
            }
            i++;
        }
        free(compose_str);
    }


#ifdef DATADIR
    installdir=DATADIR;
#endif
    if (installdir)
    {
        i=0;
        /* now try a few variants */
        while(dir_tries_pre[i] != NULL)
        {
            j=0;
            while(dir_tries_post[j] != NULL)
            {
                snprintf(manpage_dir, PATH_MAX,
                         "%s/%s/%s/suq.1", installdir,
                         dir_tries_pre[i],
                         dir_tries_post[j]);
                if (access(manpage_dir, R_OK) == 0)
                {
                    return manpage_dir;
                }
                j++;
            }
            i++;
        }
    }

    free(manpage_dir);
    return NULL;
}

void display_man_page(const char *argvzero)
{
    char *manpage;

    manpage=find_manpage(argvzero);

    /*printf("%s\n", manpage);*/
    if (manpage)
    {
        execlp("man", "man", manpage, NULL);
    }
    /* if we're still here the exec failed */
    printf("%s\n", usage_string);
}


