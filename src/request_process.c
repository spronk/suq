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

/* gets the argument i from the request r, and assigns it to a. 

NOTE: r and a can be evauluated multiple times. */

static char *request_get_arg(request *r, int arg_ind)
{
    if ( (r->argc) <= arg_ind ) 
    {
        request_reply_errstring((r), "Command error"); 
        return NULL;
    }
    return r->argv[arg_ind];
}


#if 0
#define request_get_arg(r, i, a) {\
    int ii=i; /* to avoid double evaluation */ \
    if ( ((r)->argc) <= ii ) \
    { \
        request_reply_errstring((r), "Command error"); \
        goto err; \
    }\
    (a) = (r)->argv[ii];\
}
#endif
  

void request_run(request *r, suq_serv *cs)
{
    int i;
    int arg_ind;
    int cont=1;
    char *wd=r->wd;
    job *j=malloc_check_server(sizeof(job));
    char *arg;

    job_init(j);

    j->id=suq_settings_get_next_id(cs->st);

    j->buf=malloc_check_server(r->buflen+1);
    /* just copy the whole buffer. This buffer will contain all
        variables that the job's cmd, argv[] etc. point to. */
    memcpy(j->buf, r->buf, r->buflen);

    arg_ind=2; /* the starting index to check */

    j->ntask=1;
    /* process arguments first */
    do
    {
        arg=request_get_arg(r, arg_ind);
        if (!arg) goto err;

        if (strcmp(arg, "-d") == 0)
        {
            wd=request_get_arg(r, ++arg_ind);
            if (!wd) goto err;
            ++arg_ind;
        }
        else if (strcmp(arg, "-n") == 0)
        {
            char *nps;
            char *end;
            int ntask;

            nps=request_get_arg(r, ++arg_ind);
            if (!nps) goto err;
            ntask=strtol(nps, &end, 10);
            if (end==nps || ntask<1)
            {
                request_reply_errstring(r, "suq run -n is not a number > 1");
                goto err;
            }
            /* we don't unset a blocking attribute */
            if (j->ntask>=1)
                j->ntask=ntask;
            ++arg_ind;
        }
        else if (strcmp(arg, "-p") == 0)
        {
            char *nps;
            char *end;

            nps=request_get_arg(r, ++arg_ind);
            if (!nps) goto err;
            j->prio=strtol(nps, &end, 10);
            if (end==nps)
            {
                request_reply_errstring(r, "suq run -p is not a number");
                goto err;
            }
            ++arg_ind;
        }
        else if (strcmp(arg, "-b")==0)
        {
            j->ntask=-1;
            ++arg_ind;
        }
        else
            cont=0;
    } while(cont);



    /* then make all the pointers */
    j->argc=r->argc-arg_ind;
    j->envc=r->envc;
    j->argv=malloc_check_server(sizeof(char*)*(j->argc+1));
    j->envp=malloc_check_server(sizeof(char*)*(j->envc+1));

    j->wd = j->buf + (wd - r->buf);
    arg=request_get_arg(r, arg_ind);
    if (!arg) goto err;
    j->cmd = j->buf + (arg - r->buf);

    /* the argvs */
    for(i=arg_ind;i<r->argc;i++)
    {
        arg=request_get_arg(r, i);
        if (!arg) goto err;
        j->argv[i-arg_ind] = j->buf + (arg - r->buf);
    }
    j->argv[j->argc]=NULL;

    /* the envps */
    for(i=0;i<j->envc;i++)
    {
        j->envp[i] = j->buf + (r->envp[i] - r->buf);
    }
    j->envp[j->envc]=NULL;

    /* now construct everything from this data */
    job_reinit(j);

    /* then process the job */
    j->state=waiting;

    joblist_add(&(cs->jl), j);
    joblist_check_run(&(cs->jl), cs);

    /* print result */
    request_reply_printf(r, "Submitted job id %d: '%s'. ", j->id, j->name);

    if (j->state == running)
        request_reply_printf(r,"Job is running.\n");
    else if (j->state == waiting)
        request_reply_printf(r,"Job is waiting to run.\n");
    else if (j->state == resource_error || j->state == run_error)
        request_reply_printf(r,"\nJob ERROR: '%s'.\n", j->error_string);

    if (debug>1)
    {
        printf("SERVER: new job id=%d, ntask=%d, name=%s, cmd=%s", 
               j->id, j->ntask, j->name, j->cmd);
        printf(", argc=%d, envc=%d, wd=%s\n", j->argc, j->envc, j->wd);
    }

    return;
err:
    if (j->buf)
        free(j->buf);
    if (j->argv)
        free(j->argv);
    free(j);
}

void request_del(request *r, suq_serv *cs)
{
    job *j;
    int search_id;
    char *arg;
    int found=0;

    arg=request_get_arg(r, 2);
    if (!arg) goto err;

    if ( strcmp(arg, "all") == 0)
    {
        search_id=-1;
    }
    else
    {
        char *end;
        search_id=strtol(arg, &end, 10);
        if (end==arg)
        {
            request_reply_errstring(r, "del argument is not a number");
            return;
        }
    }
   
    j=joblist_first(&(cs->jl));
    while(j)
    {
        job *jnext=joblist_next(&(cs->jl), j);
        if (search_id == -1 || j->id == search_id)
        {
            int id=j->id;
            /* here we actually remove the job from the list */
            if (! (j->state == running) )
            {
                joblist_remove(&(cs->jl), j);
                request_reply_printf(r, "Removed job id %d\n", id);
            }
            else
            {
                job_cancel(j);
                request_reply_printf(r, "Killed job id %d\n", id);
            }
            found=1;
        }
        j=jnext;
    }
    if (!found)
        request_reply_printf(r, "ERROR: Job not found\n");
    return;
err:
    return;
}


void request_pri(request *r, suq_serv *cs)
{
    job *j;
    int search_id;
    char *arg;
    int found=0, changed=0;
    char *end;
    int newpri;

    arg=request_get_arg(r, 2);
    if (!arg) goto err;

    if ( strcmp(arg, "all") == 0)
    {
        search_id=-1;
    }
    else
    {
        search_id=strtol(arg, &end, 10);
        if (end==arg)
        {
            request_reply_errstring(r, "pri id argument is not a number");
            return;
        }
    }

    arg=request_get_arg(r, 3);
    if (!arg) goto err;

    newpri=strtol(arg, &end, 10);
    if (end==arg)
    {
        request_reply_errstring(r, "pri priority argument is not a number");
        return;
    }
   
    j=joblist_first(&(cs->jl));
    while(j)
    {
        job *jnext=joblist_next(&(cs->jl), j);
        if (search_id == -1 || j->id == search_id)
        {
            int oldpri=j->prio;
            if (oldpri != newpri)
            {
                j->prio=newpri;
                joblist_re_place(&(cs->jl), j);
                request_reply_printf(r, 
                                     "Job id %d priority set from %d to %d\n", 
                                     j->id, oldpri, newpri);
                changed=1;
            }            
            found=1;
        }
        j=jnext;
    }
    if (!found)
        request_reply_printf(r, "ERROR: Job not found\n");
    else if (!changed)
        request_reply_printf(r, "No job priority changed\n");

    return;
err:
    return;
}



void request_info(request *r, suq_serv *cs)
{
    job *j;
    int search_id;
    char *arg;
    int found=0;

    arg=request_get_arg(r, 2);
    if (!arg) goto err;

    if ( strcmp(arg, "all") == 0)
    {
        search_id=-1;
    }
    else
    {
        char *end;
        search_id=strtol(arg, &end, 10);
        if (end==arg)
        {
            request_reply_errstring(r, "info argument is not a number");
            return;
        }
    }
  
    j=joblist_first(&(cs->jl)); 
    while(j)
    {
        job *jnext=joblist_next(&(cs->jl), j);
        if (search_id == -1 || j->id == search_id)
        {
            char timestr[26];
            char *loc;

            ctime_r(&(j->sub_time), timestr);
            loc=strchr(timestr, '\n'); /* remove newline */
            if (loc)
                *loc=0;

            /* we found one */
            request_reply_printf(r, "Name:                 %s\n", j->name);
            request_reply_printf(r, "Job id:               %d\n", j->id);
            request_reply_printf(r, "Priority:             %d\n", j->prio);
            request_reply_printf(r, "State:                %s\n", 
                                 job_state_strings[j->state]);
            request_reply_printf(r, "Submit time:          %s\n", timestr);
            if (j->state==running || j->state==started)
            {
                ctime_r(&(j->start_time), timestr);
                loc=strchr(timestr, '\n'); /* remove newline */
                if (loc)
                    *loc=0;
                request_reply_printf(r, "Start time:           %s\n", timestr);
                request_reply_printf(r, "Process id:           %d\n", j->pid);
            }
            if (j->state==run_error || j->state==resource_error)
            {
                request_reply_printf(r, "Error string:         %s\n", 
                                     j->error_string);
            }

            request_reply_printf(r, "Nr. of tasks:         %d\n", j->ntask);
            request_reply_printf(r, "Command:              %s\n", j->cmd);
            request_reply_printf(r, "Nr. of args:          %d\n", j->argc);
            request_reply_printf(r, "Nr. of env vars:      %d\n", j->envc);
            request_reply_printf(r, "Working directory:    %s\n", j->wd);
            request_reply_printf(r, "\n");
            found=1;
        }
        j=jnext;
    }
    if (!found)
        request_reply_printf(r, "ERROR: Job not found\n");
    return;
err:
    return;
}


void request_list(request *r, suq_serv *cs)
{
    job *j;
    int i=0;
    int n_running=0;

    j=joblist_first(&(cs->jl)); 
    while(j)
    {
        if (j->state == running)
            n_running += j->ntask;
        j=joblist_next(&(cs->jl), j);
    }
    request_reply_printf(r,"running tasks: %4d\n", n_running);
    request_reply_printf(r,"max tasks:     %4d\n", cs->st->ntask);

    /* walk the list, so earlier jobs are printed first */
    request_reply_printf(r,"%4s %4s %7s %5s %s\n", "ID", "PRIO", "STATE",     
                         "NTASK", "NAME");
    j=joblist_first(&(cs->jl)); 
    while(j)
    {
#define TASKSTRLEN 10
        char taskstr[TASKSTRLEN];
    
        if (j->ntask > 0)
            snprintf(taskstr, TASKSTRLEN, "%5d", j->ntask);
        else
            snprintf(taskstr, TASKSTRLEN, "%5s", "block");

        request_reply_printf(r,"%4d %4d %7s %5s '%s'\n", j->id, j->prio,
                             job_state_strings[j->state], taskstr,
                             j->name);
        i++;
        j=joblist_next(&(cs->jl), j);
    }

    if (i==0)
        request_reply_printf(r,"   No jobs.\n");
}

void request_ntask(request *r, suq_serv *cs)
{
    long ntask;
    char *arg;
    char *end;

    arg=request_get_arg(r, 2);
    if (!arg) goto err;

    ntask=strtol(arg, &end, 10);
    if (end==arg)
    {
        request_reply_errstring(r, "ntask argument is not a number");
        return;
    }


    suq_settings_set_ntask(cs->st, ntask);
    joblist_check_ntask(&(cs->jl), cs);
    request_reply_printf(r,"Maximum number of tasks is set to: %d\n", 
                         cs->st->ntask);
    return;
err:
    return;
}


void request_wait(request *r, suq_serv *cs)
{
    jobwait *jw=NULL;
    char *arg;
    char *end;

    jw=malloc_check_server(sizeof(jobwait));
    if (r->argc > 2)
    {
        arg=request_get_arg(r, 2);
        if (!arg) goto err;

        if ( strcmp(arg, "all") == 0)
        {
            jw->type = jw_all;
        }
        else
        {
            jw->id = strtol(arg, &end, 10);
            jw->type = jw_id;
            if (end==arg)
            {
                request_reply_errstring(r, "wait id argument is not a number");
                return;
            }
        }
    }
    else
    {
        jw->last_sub_time = time(NULL);
        jw->type = jw_time;
    }
    jw->conn = r->conn;

    if (!joblist_wait_check_finished(&(cs->jl), jw) )
    {
        joblist_wait_add(&(cs->jl), jw);
        jw->conn->keep_alive=1; 
        request_reply_printf(r, "Waiting...\n");
    }
    else
    {
        request_reply_printf(r, "Nothing to wait for.\n");
    }

    return;
err:
    if (jw)
        free(jw);
    return;
}



