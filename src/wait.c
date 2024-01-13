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
#include <unistd.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "log_err.h"
#include "wait.h"


void jobwait_init(jobwait *jw)
{
    jw->next = NULL;
    jw->conn = NULL;
}

void jobwait_destroy(jobwait *jw)
{

}

#define MAXREPLEN 1024
void jobwait_close_connection(jobwait *jw, conn_list *cl)
{
    char outstring[MAXREPLEN];
    /*char timestr[26];*/
    switch(jw->type)
    {
        case jw_id:
            snprintf(outstring, MAXREPLEN, "Finished job id %d.\n", jw->id);
            break;
        case jw_all:
            snprintf(outstring, MAXREPLEN, "Finished all jobs.\n");
            break;
        case jw_time:
#if 0
            ctime_r( &(jw->last_sub_time), timestr);
            snprintf(outstring, MAXREPLEN,
                     "Finished jobs submitted before %s", timestr);
#endif
            snprintf(outstring, MAXREPLEN, "Finished all pending jobs.\n");
            break;
    }
    if (jw->conn)
    {
        if (write(jw->conn->write_fd, outstring, strlen(outstring)+1)<0)
        {
            server_error("Write to client failed");
        }
        conn_list_remove(cl, jw->conn);
        jw->conn=NULL;
    }
}


