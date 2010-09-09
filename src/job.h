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


#ifndef __JOB_H__
#define __JOB_H__

#include "connection.h"
#include "wait.h"

extern const char *job_state_strings[];

typedef struct job
{
    int id; /* job id */

    int prio; /* the priority */ 
    
    int ntask; /* the number of processors used */


    char *name; /* job name */
    char *wd; /* the working directory */
    char *cmd; /* the command */
    int argc; /* the number of arguments */
    char **argv;  /* the arguments */
    int envc; /* the number of environment vars */ 
    char **envp; /* the environment vars */ 

    char *stdout_filename; /* the filename for stdout+stderr (relative to wd) */

    /* run params */
    pid_t pid; /* process id */

    /* job state. Must match job_state_strings */
    enum job_state 
    { 
        run_error,          /* an unfixble run error. */
        resource_error,     /* ntask > global ntask */
        waiting,            /* process is waiting */
        started,            /* process is started */
        running,            /* process is running */
        done                /* process is done */
    } state;

    int run_order; /* the order in which the job was started */
    time_t sub_time; /* time at which the job was submitted */
    time_t start_time; /* time at which the job was started */
    time_t end_time; /* time at which the job was finished */
       
    char *error_string;    

    /* misc  */
    struct job *next, *prev;  /* prev and next jobs */
    char *buf; /* the main buffer containing these data */
} job;


/* the job list is a simple-minded orderd linked list as a priority queue.
   We don't expect N to be so large that its O(N) behavior on inserts 
   will be a problem */
typedef struct
{
    job *head; /* the head of the job list - is a dummy element */
    int N;

    job head_elem; /* the pre-allocated head element */

    int run_id; /* the run id for the next job */

    jobwait *wait_head; /* head of the wait list - a dummy element */

    jobwait wait_head_elem; /* pre-allocated wait head element */
} joblist;

struct suq_serv;


/* initialize the job list */
void joblist_init(joblist *jl);
/* destroy the job list contents */
void joblist_destroy(joblist *jl);

/* add a job to the list  */
void joblist_add(joblist *jl, job *j);

/* remove a job from the list */
void joblist_remove(joblist *jl, job *j);

/* give a job a new place on the list based on its new state. */
void joblist_re_place(joblist *jl, job *j);

/* add a wait to a joblist */
void joblist_wait_add(joblist *jl, jobwait *jw);

/* remove a wait from a joblist */
void joblist_wait_remove(joblist *jl, jobwait *jw);

/* cancel a wait from a joblist after a connection close*/
void joblist_wait_cancel(joblist *jl, jobwait *jw);

/* check whether there is a wait that has finished */
void joblist_wait_check_finished_all(joblist *jl, conn_list *cl);

/* check whether there is a wait that has finished */
int joblist_wait_check_finished(joblist *jl, jobwait *jw);

/* search for a wait associated with a specific connection */
jobwait *joblist_wait_search_conn(joblist *jl, connection *cn);


/* get the number of jobs in the list */
int joblist_N(joblist *jl);

/* check whether we can run any jobs, and run them.  */
void joblist_check_run(joblist *jl, struct suq_serv *srv);

/* Check whether a new total ntask leads to new resource_errors, or
   solves old ones */
void joblist_check_ntask(joblist *jl, struct suq_serv *srv);



/* get the highest-priority job, or NULL if there's no jobs */
job *joblist_first(joblist *jl);
/* get the next-hightest-priority job, or NULL when there's no jobs left */
job *joblist_next(joblist *jl, job *j);




/* create a new job */
void job_init(job *j);
/* destroy a job */
void job_destroy(job *j);

/* returns 1 if ja has higher run priority than jb */
int job_gt(job *ja, job *jb);

/* run a job */
void job_run(job *j, int run_id);

/* cancel a job if it's running */
void job_cancel(job *j);

/* recycle  */
void job_reinit(job *j);

#endif

