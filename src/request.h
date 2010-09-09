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


#ifndef __REQUEST_H__
#define __REQUEST_H__

#define REPLY_SIZE 10*1024



/* process an actual request from a specific connection, possibly changing
   server state cs */
void request_process(connection *c, suq_serv *cs);

typedef struct request 
{
    char *wd; /* the client working directory */
    int argc; /* the number of client arguments to the request */
    char **argv; /* the client arguments */
    int envc; /* the number of client environment variables */
    char **envp; /* the client environment variables */

    char *buf; /* buffer to the raw request, into which wd, argv, and 
                  environ point. */
    size_t buflen; /* the buffer length */

    char *reply; /* the request reply */
    size_t reply_alloc; /* number of allocated bytes to reply */
    size_t reply_size;

    connection *conn; /* the associated connection */
} request;

/* create a request from connection data */
void request_init(request *r, connection *c);
/* deallocate everything connected to a request */
void request_destroy(request *r);

/* replace the reply string with an error message, and a usage string */
void request_reply_errstring(request *r, const char *message);
/* print fmt, etc. to the reply */
void request_reply_printf(request *r, const char *fmt, ...);


/* process a run request */
void request_run(request *r, suq_serv *cs);
/* process a del request */
void request_del(request *r, suq_serv *cs);
/* process a pri request */
void request_pri(request *r, suq_serv *cs);
/* process a list request */
void request_list(request *r, suq_serv *cs);
/* process an info request */
void request_info(request *r, suq_serv *cs);
/* process a nproc request */
void request_nproc(request *r, suq_serv *cs);
/* process a ntask request */
void request_ntask(request *r, suq_serv *cs);
/* process a wait request */
void request_wait(request *r, suq_serv *cs);

#endif
