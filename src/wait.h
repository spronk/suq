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

#ifndef __WAIT_H__
#define __WAIT_H__

#include <time.h>

#include "connection.h"

typedef enum
{
    jw_id,      /* wait until a specific job id has finished */
    jw_time,    /* wait until a all jobs submitted earlier than some
                   specified time have finished */
    jw_all      /* wait until the job queue is empty */
} jobwait_type;

/* wait structure. */
typedef struct jobwait
{
    jobwait_type type;

    int id; /* the id to wait for */
    time_t last_sub_time; /* the submission time to check for */

    struct connection *conn;

    struct jobwait *next, *prev;
} jobwait;

void jobwait_init(jobwait *jw);

/* remove data from jobwait. Assumes connection is closed */
void jobwait_destroy(jobwait *jw);

/* close the wait and the associated connection */
void jobwait_close_connection(jobwait *jw, conn_list *cl);


#endif /* __WAIT_H__ */

