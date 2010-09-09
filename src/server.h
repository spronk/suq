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


#ifndef __SERVER_H__
#define __SERVER_H__

#include "connection.h"
#include "job.h"

#include <signal.h>

/* the server state */
typedef struct suq_serv
{
    int sockdes; /* the listening socket */
    conn_list cl; /* the active connections */
    joblist jl; /* the running jobs */
    /*int Nproc; *//* max number of processors to run on */
    /*unsigned int next_id; *//* next job id */

    /*char *sock_filename;*/
    suq_settings *st; /* settings */
} suq_serv; 

/* a global variable to allow signal handler access */
extern suq_serv cs;
extern volatile sig_atomic_t sig_check;


/* main loop of the suq_serv */
void suq_serv_main(suq_settings *st, int pipe_in, int pipe_out);

/* initialize suq_serv structure */
void suq_serv_init(suq_serv *cs, suq_settings *st, int pipe_in, int pipe_out);

/* destroy suq_serv structure contents */
void suq_serv_destroy(suq_serv *cs);

/* accept connection */
void suq_serv_accept_connection(suq_serv *cs);

/* wait for finished processes after we got a signal */
void suq_serv_wait_proc(suq_serv *cs);

#endif


