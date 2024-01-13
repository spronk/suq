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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/un.h>
#include <errno.h>
#include <limits.h>
#include <paths.h>
#include <fcntl.h>
#include <sys/stat.h>



#include "log_err.h"
#include "srv_config.h"
#include "client_conn.h"
#include "server.h"

extern char **environ;

int client_try_connection(client_connection *cc, suq_config *sc)
{
    int sockdes;
    struct sockaddr_un server_addr;
    int connres;

    /* first try to connect to a listening server */
    sockdes=socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockdes < 0)
        fatal_system_error("Opening socket");

    if (debug>0)
        printf("CLIENT: Trying to connect to %s\n", sc->socket_filename);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, sc->socket_filename,
            sizeof(server_addr.sun_path));
    connres=connect(sockdes,
                    (const struct sockaddr*)&server_addr,
                    SUN_LEN(&server_addr));

    if (connres >= 0)
    {
        return sockdes;
    }
    return 0;
}

void client_connection_init(client_connection *cc, suq_config *sc)
{
    int sockdes = client_try_connection(cc, sc);

    if (sockdes != 0)
    {
        /* we're connected to a server */
        if (debug>0)
            printf("CLIENT: Connecting to existing server\n");
        cc->fd_read=sockdes;
        cc->fd_write=sockdes;
    }
    else
    {
        /* there was no server. We fork with two pipe pairs for
           immediate communication. */
        int fdes_inp[2]; /* input pipe pair from server */
        int fdes_out[2]; /* output pipe pair to server  */
        int pid;
        char *logname;
        int stdo; /* the stdout replacement file */
        int stdi; /* the stdin replacement file */

        if (debug>0)
            printf("CLIENT: No server, spawning new one. Error was '%s'\n",
                   strerror(errno));

        if (sockdes > 0)
            close(sockdes);
        /* open pipe pairs*/
        if (pipe(fdes_out)<0)
            fatal_system_error("cs queuer pipe creation");
        if (pipe(fdes_inp)<0)
            fatal_system_error("cs queuer pipe creation");

        /* we are committed to the paths so we make them now. */
        suq_config_makedirs(sc);

        /* open the log file name */
        logname = suq_config_get_server_log_name(sc);
        stdo=open(logname, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
        if (stdo<0)
        {
            char msg[MAXPATHLEN + 100];
            snprintf(msg, MAXPATHLEN + 99, "cs queuer log file %s", logname);
            fatal_system_error(msg);
        }
        free(logname);

        stdi=open("/dev/null", O_RDONLY);
        if (stdi<0)
        {
            fatal_system_error("cs queuer stdin file /dev/null");
        }

        pid=fork();
        if (pid<0)
            fatal_system_error("cs queuer fork");

        if (pid==0)
        {
            /* I am now almost the server; we fork again to have no parent. */
            int ret=fork();

            if (ret==0)
            {
                /* I am the server. */
                close(fdes_inp[0]); /* input from server is now output for server */
                close(fdes_out[1]); /* output to server is now input for server */

                /* we go to the root dir because we're now a daemon */
                if (chdir("/") < 0)
                    fatal_system_error("chdir to / failed");

                /* we try to become a new session leader */
                ignore_error(setsid());

                /* take care of the stdin/out/err files */
                close(STDOUT_FILENO);
                dup2(stdo, STDOUT_FILENO);
                close(STDERR_FILENO);
                dup2(stdo, STDERR_FILENO);
                close(STDIN_FILENO);
                dup2(stdi, STDIN_FILENO);
                close(stdi);
                close(stdo);


                /* main loop of the server. Potentially never returns. */
                suq_serv_main(sc, 0, fdes_out[0], fdes_inp[1]);
                suq_config_destroy(sc);
            }
            exit(0);
        }
        else
        {
            /* I am still the client */
            close(fdes_inp[1]);
            close(fdes_out[0]);
            cc->fd_read=fdes_inp[0];
            cc->fd_write=fdes_out[1];
        }
    }

    /* now we have a working client_connection */
}

void client_connection_finish_sending(client_connection *cc)
{
}

void client_connection_destroy(client_connection *cc)
{
    close(cc->fd_write);
    close(cc->fd_read);
}

void client_connection_send_request(client_connection *sc, int argc,
                                    char *argv[])
{
    char *buf=NULL;
    size_t needed=0;
    size_t cursor=0;
    int i;

    for(i=0; i<argc; i++)
    {
        needed += strlen(argv[i])+1;
    }
    i=0;
    needed++; /* for the nul separator */
    while(environ[i])
    {
        needed += strlen(environ[i])+1;
        i++;
    }
    needed++; /* for the nul separator */
    needed+=MAXPATHLEN+1; /* for the current working directory */
    needed++; /* for the nul separator */
    needed+=3; /* for the end double nul separator */

    buf=malloc_check(sizeof(char)*needed);
    /* first we have the current working directory */
    if (! getcwd(buf, MAXPATHLEN) )
        server_system_error("getcwd");

    cursor=strlen(buf)+1;
    buf[cursor++]=0;

    /* then, we copy the arguments: */
    for(i=0; i<argc; i++)
    {
        int len=strlen(argv[i])+1;
        strncpy(buf+cursor, argv[i], len);
        cursor += len;
    }
    buf[cursor++]=0;
    /* and finally, the environment */
    i=0;
    while(environ[i])
    {
        int len=strlen(environ[i])+1;
        strncpy(buf+cursor, environ[i], len);
        cursor += len;
        i++;
    }
    buf[cursor++]=0;
    buf[cursor++]=0;

    if (write(sc->fd_write, buf, cursor) < 0)
        fatal_system_error("server write");
}

void client_connection_get_print_results(client_connection *sc, int *errcode)
{
    size_t alloc_size=0;
    char *buf=NULL;
    int ret;
    FILE *outfile;

    *errcode=0;
    do
    {
        size_t cursor=0;
        size_t read_size;
        if (alloc_size>0)
            buf[0]=0;
        do
        {
            /* allocate if needed */
            if (alloc_size <= cursor)
            {
                if (debug>1)
                    printf("CLIENT: Allocating read buffer\n");
                alloc_size+=CLIENT_CONN_BUF_SIZE;
                buf=realloc_check(buf, alloc_size);
            }
            /* do the read */
            read_size=alloc_size-cursor;
            ret=read(sc->fd_read, buf+cursor, read_size);
            if (debug>1)
                printf("CLIENT: Read ret=%d\n",ret);
            if (ret < 0)
                fatal_system_error("read");
            cursor+=ret;
        }
        /* read as long as there's something to be read */
        while (ret==read_size);

        if (strncmp(buf, "ERROR", strlen("ERROR")) == 0)
        {
            *errcode=1;
            outfile=stderr;
        }
        else
        {
            outfile=stdout;
        }
        fprintf(outfile, "%s", buf);
    }
    while (ret>0);

    free(buf);

    close(sc->fd_read);
}
