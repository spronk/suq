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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usage.h"
#include "log_err.h"
#include "srv_config.h"
#include "client_conn.h"
#include "server.h"

/* one global variable made available across fork() */
suq_config sc;


/* get the next argument, updating counter i, with name optname, type opttype
   for error message purposes */
static const char *get_next_argv(int *i, int argc, char *argv[],
                                 const char *optname, const char *opttype)
{
    (*i)++;
    if (argc < (*i + 1))
    {
        fprintf(stderr, "ERROR: no %s specified with %s\n", opttype, optname);
        fprintf(stderr, "%s", usage_string);
        exit(EXIT_FAILURE);
    }
    return argv[*i];
}


int main(int argc, char *argv[])
{
    client_connection cc;
    int errcode=EXIT_SUCCESS;
    int i,j;
    int detach=1;
    const char *config_filename_input = getenv("SUQ_CONFIG_FILE");
    char *config_filename;

    /* check for client arguments */
    for(i=1;i<argc;i++)
    {
        if (strcmp(argv[i], "-d")==0 || strcmp(argv[i], "--debug")==0)
        {
            detach=0;
            debug=2;
        }
        else if (strcmp(argv[i], "-c")==0)
        {
            debug=2;
        }
        else if (strcmp(argv[i], "--config")==0)
        {
            config_filename_input = get_next_argv(&i, argc, argv,
                                                  "--settings", "filename");
        }
        else if (strcmp(argv[i], "-h")==0)
        {
            fprintf(stdout, "%s", usage_string);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argv[i], "help") == 0) /* a special case */
        {
            display_man_page(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argv[i], "-v")==0)
        {
#ifdef PACKAGE_STRING
            fprintf(stdout, "%s\n", PACKAGE_STRING);
#else
            fprintf(stdout, "%s\n", "Unknown version");
#endif
            exit(EXIT_SUCCESS);
        }
        else
            break;
    }
    /* now we shift the argvs back so we can push those to the server */
    if (i>1)
    {
        for(j=1;j<(argc+(i-1));j++)
        {
            argv[j]=argv[j+(i-1)];
        }
        argc-=(i-1);
    }

    /* Initialize config */
    config_filename = suq_config_get_filename(config_filename_input);

    /* Initialize the config object */
    suq_config_init(&sc, config_filename);
    free(config_filename);

    if (detach)
    {
        /* connect to an already existing daemon, or spawn a new one */
        client_connection_init(&cc, &sc);
        /* the client is stupid: the daemon does all the work, including
           parsing the command line. */
        client_connection_send_request(&cc, argc, argv);

        /* we just read back the results */
        client_connection_get_print_results(&cc, &errcode);
        /* and close the connection */
        client_connection_destroy(&cc);
    }
    else
    {
        /* we just start the server main, but first we check whether another
           server is running */
        int sockdes = client_try_connection(&cc, &sc);

        if (sockdes != 0)
        {
            fatal_error("Trying to run in non-detached mode, but a server is "
                        "alrady running");
        }
        suq_serv_main(&sc, -1, -1, 1);
    }
    suq_config_destroy(&sc);

    return errcode;
}
