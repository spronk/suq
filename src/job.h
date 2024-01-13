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


#ifndef __JOB_H__
#define __JOB_H__

#include "connection.h"
#include "wait.h"

extern const char *job_state_strings[];

/** Descriptor for a single job */
typedef struct job
{
    int id;                 /** job id */

    int prio;               /** the priority */

    int ntask;              /** the number of processors used */


    char *name;             /** job name */
    char *wd;               /** the working directory */
    char *cmd;              /** the command */
    int argc;               /** the number of arguments */
    char **argv;            /** the arguments */
    int envc;               /** the number of environment vars */
    char **envp;            /** the environment vars */

    char *stdout_filename;  /** the filename for stdout+stderr
                               (relative to wd) */

    /* run params */
    pid_t pid;              /** process id */

    enum job_state
    {
        run_error,      /** an unfixble run error. */
        resource_error, /** ntask > global ntask */
        waiting,        /** process is waiting */
        started,        /** process is started */
        running,        /** process is running */
        done,           /** process is done */
        canceled,       /** process is canceled */
    } state;                /** job state. Must match job_state_strings */

    int run_order;          /** the order in which the job was started */

    /* TODO: higher resolution time based on clock_gettime(CLOCK_MONOTONIC) */
    time_t sub_time;        /** time at which the job was submitted */
    time_t start_time;      /** time at which the job was started */
    time_t end_time;        /** time at which the job was finished */

    double duration;        /** total job duration in seconds */

    int return_status;      /** return status (valid when state == done) */

    char *error_string;     /** error string in case of
                                job_state == (run_error|resource_error) */

    /* misc  */
    struct job *next,       /** next job in linked list */
               *prev;       /** prev job in linked list */
    char *buf;              /** the main buffer containing these data */
} job;


/** the job list is a simple-minded orderd linked list as a priority queue.
    We don't expect N to be so large that its O(N) behavior on inserts
    will be a problem */
typedef struct
{
    job *head;              /** the head of the job list - is a dummy element */
    int N;                  /** Total number of jobs */

    job head_elem;          /** the pre-allocated head element */

    int run_id;             /** the run id for the next job */

    jobwait *wait_head;     /** head of the wait list - a dummy element */

    jobwait wait_head_elem; /** pre-allocated wait head element */
} joblist;

struct suq_serv;


/** initialize the job list
 *
 * @param jl    Pointer to the allocated joblist
 */
void joblist_init(joblist *jl);

/** destroy the job list contents
 *
 * @param jl    The joblist whose contents to destroy
 */
void joblist_destroy(joblist *jl);

/** add a job to the list
 *
 * @param jl    The joblist to add to
 * @param j     The job to add
 */
void joblist_add(joblist *jl, job *j);

/** remove a job from the list
 *
 * @param jl    The joblist to remove from
 * @param j     The job to remove
 */
void joblist_remove(joblist *jl, job *j);

/** give a job a new place on the list based on its new state.
 *
 * @param jl    The joblist to alter
 * @param j     The job to re-place
 */
void joblist_re_place(joblist *jl, job *j);

/** add a wait to a joblist
 *
 * @param jl    The joblist to attach a jobwait object to
 * @param jw    The jobwait object to attach
 */
void joblist_wait_add(joblist *jl, jobwait *jw);

/** remove a wait from a joblist
 *
 * @param jl    The joblist to remove a jobwait object from
 * @param jw    The jobwait object to remove
 */
void joblist_wait_remove(joblist *jl, jobwait *jw);

/** cancel a wait from a joblist after a connection close
 *
 * @param jl    The joblist to cancel a wait for
 * @param jw    The jobwait to cancel
 */
void joblist_wait_cancel(joblist *jl, jobwait *jw);

/** check whether there is a wait that has finished
 *
 * @param jl        The joblist to check
 * @param conn_list The connection list to use in the check
 */
void joblist_wait_check_finished_all(joblist *jl, conn_list *cl);

/** check whether there is a wait that has finished
 *
 * @param jl        The joblist to check
 * @param conn_list The connection list to use in the check
 * @returns         1 if all waits are satisfied, 0 if not
 */
int joblist_wait_check_finished(joblist *jl, jobwait *jw);

/** search for a wait associated with a specific connection */
jobwait *joblist_wait_search_conn(joblist *jl, connection *cn);


/** get the number of jobs in the list
 *
 * @param jl    The joblist to check
 * @returns     The number of jobs in jl
 */
int joblist_N(joblist *jl);

/** check whether we can run any jobs, and run them.
 *
 * @param jl    The joblist to check
 * @param srv   The server definition
 */
void joblist_check_run(joblist *jl, struct suq_serv *srv);

/** Check whether a new total ntask leads to new resource_errors, or
 * solves old ones.
 *
 * @param jl    The joblist
 * @param srv   The server definition
 * @returns     1 if there are resource errors, 0 if not.
 */
int joblist_check_ntask(joblist *jl, struct suq_serv *srv);



/** get the highest-priority job, or NULL if there's no jobs
 *
 * @param jl    The joblist
 * @returns     The highest-priority job
 */
job *joblist_first(joblist *jl);

/** get the next-hightest-priority job, or NULL when there's no jobs left
 *
 * @param jl    The joblist
 * @param j     The job whose next-highest-priority job to get
 * @returns     The job that is the next-highest-priority relative to j
 */
job *joblist_next(joblist *jl, job *j);




/** create a new job
 *
 * @param j     The pre-allocated job to initalize
 */
void job_init(job *j);

/** destroy a job
 * @param j     The job whose contenst to destroy
 */
void job_destroy(job *j);

/** returns 1 if ja has higher run priority than jb
 *
 * @param ja    Job a
 * @param jb    Job b
 * @returns     1 if priority(ja) > priority(jb), 0 otherwise
*/
int job_gt(job *ja, job *jb);

/** run a job
 *
 * @param j         The job to run
 * @param run_order The order in which the job is run
*/
void job_run(job *j, int run_order);

/** update a job to a finished state
 *
 * @param j             The job
 * @param return_status The job process' return status
*/
void job_finish(job *j, int return_status);

/** cancel a job if it's running
 *
 * @param j     The job to cancel (kill the process if it's running)
*/
void job_cancel(job *j);

/** prepare for submission
 *
 * @param j             The job object to re-initialize
 * @param output_dir    The job's stdout output dir.
*/
void job_prep(job *j, const char *output_dir);

/* write a log entry for a finished job.
 *
 * @param j         The job
 * @param outfile   The job log output file
*/
void job_write_log_entry(job *j, FILE *outfile);

#endif
