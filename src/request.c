/* This source code is part of 

suq, the Single-User Queuer

Copyright (c) 2010 Sander Pronk
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
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "err.h"
#include "connection.h"
#include "job.h"
#include "settings.h"
#include "server.h"
#include "request.h"
#include "usage.h"




void request_process(connection *c, suq_serv *cs)
{
    request r;

    request_init(&r, c);

    /* we assume a complete request */
    c->keep_alive=0;

    if (r.argc<2)
    {
        request_reply_errstring(&r, "no command");
    }
    else
    {
        if ( (strcmp(r.argv[1], "run")==0) || 
             (strcmp(r.argv[1], "sub")==0) )
        {
            request_run(&r, cs);
        }
        else if (strcmp(r.argv[1], "del")==0)
        {
            request_del(&r, cs);
        }
        else if (strcmp(r.argv[1], "pri")==0)
        {
            request_pri(&r, cs);
        }
        else if (strcmp(r.argv[1], "info")==0)
        {
            request_info(&r, cs);
        }
        else if (strcmp(r.argv[1], "wait")==0)
        {
            request_wait(&r, cs);
        }
        else if ((strcmp(r.argv[1], "ls")==0) || 
                 (strcmp(r.argv[1], "list")==0))
        {
            request_list(&r, cs);
        }
        else if ( (strcmp(r.argv[1], "ntask")==0)||
                  (strcmp(r.argv[1], "nproc")==0) )
        {
            request_ntask(&r, cs);
        }
        else if (strcmp(r.argv[1], "help")==0)
        {
            request_reply_printf(&r, "%s", usage_string);
        }
        else if (strcmp(r.argv[1], "echo")==0)
        {
            size_t j;

            request_reply_printf(&r, "Echo: ");
            for(j=0;j<r.argc;j++)
            {
                request_reply_printf(&r, " '%s'", r.argv[j]);
            }
        }
        else
        {
            request_reply_errstring(&r, "wrong command");
        }
    }



    if (r.reply_size > 0)
    {
        if (debug>0)
            printf("SERVER: Replying: '%s'\n", r.reply);
        if (write(c->write_fd, r.reply, r.reply_size+1) < 0)
            server_system_error("write to client failed");
    }
    else
    {
        printf("SERVER: No reply:\n");
    }
    request_destroy(&r);
}

void request_init(request *r, connection *c)
{
#define SPLIT_ALLOC 128
    size_t nalloc=0;
    size_t j; /* the read cursor */

    /* first split the request */
    r->buf=c->read_buf;
    r->buflen=c->read_buf_request;
    r->wd=r->buf; /* the zeroth string is the wd. */

    nalloc=0;
    r->argv=NULL;
    r->argc=0;

    j=strlen(r->wd)+2;
    /* now do the argvs until we hit a double nul */
    while(! (c->read_buf[j-1]==0 && c->read_buf[j]==0))
    {
        if (c->read_buf[j-1]==0 && c->read_buf[j]!=0) /* if the previous one 
                                                         was a nul */
        {
            if ( ((r->argc)+1) >= nalloc)
            {
                nalloc += SPLIT_ALLOC;
                r->argv=realloc_check_server(r->argv, sizeof(char*)*(nalloc+1));
            }
            r->argv[(r->argc)++]=c->read_buf + j;
        }
        j++;
    }
    r->argv[r->argc]=NULL;

    /* and now do the envp */
    nalloc=0;
    r->envp=NULL;
    r->envc=0;
    j++;
    while(! (c->read_buf[j-1]==0 && c->read_buf[j]==0))
    {
        if (c->read_buf[j-1]==0 && c->read_buf[j]!=0) /* if the previous one 
                                                         was a nul */
        {
            if ( ((r->envc)+1) >= nalloc)
            {
                nalloc += SPLIT_ALLOC;
                r->envp=realloc_check_server(r->envp, sizeof(char*)*(nalloc+1));
            }
            r->envp[(r->envc)++]=c->read_buf + j;
        }
        j++;
    }
    r->envp[r->envc]=NULL;

    if (debug>1)
    {
        printf("SERVER: request size: %d, %d, wd=%s:", r->argc, r->envc, r->wd);
        for(j=0;j<r->argc;j++)
            printf(" '%s'", r->argv[j]);
        printf("\n");
    }
    /* then allocate the reply */
    r->reply_alloc=REPLY_SIZE;
    r->reply_size=0;
    r->reply=malloc_check_server(r->reply_alloc*sizeof(char));
    r->conn=c;
}


void request_destroy(request *r)
{
    free(r->argv);
    free(r->envp);
    /* deallocate the reply */
    free(r->reply);
}

void request_reply_errstring(request *r, const char *message)
{
    size_t nlen=strlen(message) + strlen(usage_string) + strlen("ERROR");
    if (nlen > r->reply_alloc)
    {
        r->reply_alloc = nlen+1;
        r->reply=realloc_check_server(r->reply, r->reply_alloc);
    }

    r->reply_size=sprintf(r->reply, "ERROR: %s\n%s\n", message, 
                          usage_string);
}

void request_reply_printf(request *r, const char *fmt, ...)
{
    int nsize=0;
    size_t nleft;
    do
    {
        va_list ap;

        nleft=r->reply_alloc - r->reply_size;

        if (nsize + r->reply_size >= r->reply_alloc)
        {
            r->reply_alloc += REPLY_SIZE;
            r->reply=realloc_check_server(r->reply, 
                                          r->reply_alloc*sizeof(char));
        }

        va_start(ap, fmt);
        nsize=vsnprintf(r->reply + r->reply_size, nleft, fmt, ap);
        va_end(ap);
    } while (nsize  > nleft);
    r->reply_size += nsize;
}


