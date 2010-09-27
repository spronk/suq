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
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "err.h"
#include "wait.h"
#include "job.h"
#include "settings.h"
#include "server.h"
#include "signal.h"

#define ERRSTRING_LEN 1024

/* must match job_state enum */
const char *job_state_strings[] = { "Error", "Error", "Wait", "Started", 
                                    "Running", "Done" };

char *job_resource_error_string = 
"Requested ntask bigger than the total number available";


void joblist_init(joblist *jl)
{
    jl->head= &(jl->head_elem);
    jl->head->cmd=NULL;
    jl->head->next=jl->head; /* the head element initially just points back */
    jl->head->prev=jl->head; /* the head element initially just points back */
    jl->N=0;
    jl->run_id=0;

    jl->wait_head=&(jl->wait_head_elem);
    jl->wait_head->next = jl->wait_head; /* head elem points back */
    jl->wait_head->prev = jl->wait_head; /* head elem points back */
}

void joblist_destroy(joblist *jl)
{
    job *j=joblist_first(jl);

    while(j)
    {
        job *next=joblist_next(jl, j);
        joblist_remove(jl, j);
        j=next;
    }
}

job *joblist_first(joblist *jl)
{
    job *ret=jl->head->next;
    if (ret == jl->head)
        ret=NULL;
    return ret;
}

job *joblist_next(joblist *jl, job *j)
{
    job *ret=j->next;
    if (ret == jl->head)
        ret=NULL;
    return ret;
}



void joblist_add(joblist *jl, job *j)
{
    /* seek out our place in the queue based on our priority */
    job *job_before=joblist_first(jl);

    while(job_before && job_gt(job_before, j))
    {
        job_before=joblist_next(jl, job_before);
    }

    /* if there wasn't a job_before, we're at the bottom of the queue */
    if (!job_before)
        job_before=jl->head;

    j->prev=job_before->prev;
    j->next=job_before;

    j->next->prev=j;
    j->prev->next=j;
    jl->N++;
}

void joblist_remove(joblist *jl, job *j)
{
    if (debug>1)
        printf("SERVER: removing job id %d\n", j->id);


    j->next->prev=j->prev;
    j->prev->next=j->next;

    jl->N--;


    if (j->state == running)
        job_cancel(j);

    job_destroy(j);
    free(j);
}

void joblist_re_place(joblist *jl, job *j)
{
    /* remove it from the list */
    j->next->prev=j->prev;
    j->prev->next=j->next;
    jl->N--;

    /* and add it to the list again */
    joblist_add(jl, j);
}




void joblist_wait_add(joblist *jl, jobwait *jw)
{
    /* insert it into the list */
    jw->next=jl->wait_head->next;
    jw->prev=jl->wait_head;
    jl->wait_head->next->prev=jw;
    jl->wait_head->next=jw;
}

void joblist_wait_remove(joblist *jl, jobwait *jw)
{
    /* remove it from the list */
    jw->next->prev=jw->prev;
    jw->prev->next=jw->next;
    if (debug>1)
        printf("SERVER: joblist_wait_remove\n");

    jobwait_destroy(jw);
    free(jw);
}

void joblist_wait_check_finished_all(joblist *jl, conn_list *cl)
{
    jobwait *jw=jl->wait_head->next;

    if (debug>1)
        printf("SERVER: joblist_wait_check_finished\n");
    while(jw != jl->wait_head)
    {
        jobwait *next = jw->next;
        if (joblist_wait_check_finished(jl, jw))
        {
            if (debug>1)
                printf("SERVER: removing wait\n");
            jobwait_close_connection(jw, cl);
            joblist_wait_remove(jl, jw);
        }
        jw = next;
    }
}

int joblist_wait_check_finished(joblist *jl, jobwait *jw)
{
    int match=1;

    if (jw->type == jw_id)
    {
        /* check all the jobs for a specific id */
        job *jn=jl->head->next;
        while(jn != jl->head)
        {
            if (jn->id == jw->id)
            {
                match=0;
                break;
            }
            jn = jn->next;
        }
    }
    else if (jw->type == jw_time)
    {
        /* check all jobs for earlier ones */
        job *jn=jl->head->next;
        while(jn != jl->head)
        {
            if (jn->sub_time < jw->last_sub_time)
            {
                match=0;
                break;
            }
            jn = jn->next;
        }
    }
    else if (jw->type == jw_all)
    {
        /* count the number of jobs not including
           the one provided as argument */
        if (jl->N > 0)
            match=0;
    }
    if (debug>1)
        printf("SERVER: wait_check_finished match=%d\n", match);
    return match;
}

jobwait *joblist_wait_search_conn(joblist *jl, connection *cn)
{
    jobwait *jw=jl->wait_head->next;

    while(jw != jl->wait_head)
    {
        if (jw->conn == cn)
            return jw;
        jw = jw->next;
    }
    return NULL;
}




int joblist_N(joblist *jl)
{
    return jl->N;
}


void joblist_check_run(joblist *jl, suq_serv *srv)
{
    /* first check how many jobs are running */
    int n_running=0;
    job *j;

    if (debug>1)
        printf("SERVER: joblist_check_run\n");

    /* first count the running jobs and handle old status updates */
    j=joblist_first(jl);
    while(j)
    {
        char timestr[26];
        job *next=joblist_next(jl, j);
        char *loc;

        if (j->state == running || j->state == started)
        {
            n_running += j->ntask;
        }
        if (j->state == started) 
        {
            ctime_r(&(j->start_time), timestr);
            loc=strchr(timestr, '\n'); /* remove newline */
            if (loc)
                *loc=0;
            printf("%s: job %d (%s) started with pid %d\n", timestr, 
                   j->id, j->name, j->pid);

            /* it can only gain in priority, so we won't see it again
               later in the list. */
            j->state = running;
            joblist_re_place(jl, j);
        }
        else if (j->state == done) 
        {
            ctime_r(&(j->end_time), timestr);
            loc=strchr(timestr, '\n'); /* remove newline */
            if (loc)
                *loc=0;
            printf("%s: job %d (%s) finished\n", timestr, j->id, j->name);

            joblist_remove(jl, j);
        }
        j=next;
    }
    if (debug>1)
        printf("SERVER: n_running=%d\n", n_running);

    /* then run new jobs if there's place for them */
    j=joblist_first(jl);
    while ( j && (n_running <= srv->st->ntask) )
    {
        job *next=joblist_next(jl, j);
        int jntask=j->ntask;

        if (jntask <= 0)
            jntask = srv->st->ntask;

        if ( j->state==waiting ) 
        {
            if (n_running + jntask <= srv->st->ntask) 
            {
                job_run(j, (jl->run_id)++);
                /* it can only gain in priority, so we won't see it again */
                joblist_re_place(jl, j);
                j->state = running;
            }
            /* this makes the queue non-backfilling */
            n_running += jntask;
        }
        if (j->state==waiting && (j->ntask > srv->st->ntask) )
        {
            j->state = resource_error;
            j->error_string = job_resource_error_string;

            /* it doesn't matter if we look at it again */
            joblist_re_place(jl, j);
        }
        j=next;
    }
}



int joblist_check_ntask(joblist *jl, suq_serv *srv)
{
    int ret=0;

    job *j=joblist_first(jl);
    while( j )
    {
        job *next=joblist_next(jl, j);

        if ( j->state==waiting && (j->ntask > srv->st->ntask))
        {
            j->state = resource_error;
            j->error_string = job_resource_error_string;
            ret=1;
            joblist_re_place(jl, j);
        }
        else if ( (j->state==resource_error) && (j->ntask <= srv->st->ntask))
        {
            j->state = waiting;
            joblist_re_place(jl, j);
        }
        j=next;
    }
    return ret;
}


void job_init(job *j)
{
    j->prev=j->next=NULL;
    j->buf=NULL;
    j->argv=NULL;
    j->envp=NULL;
    j->error_string=NULL;

    j->ntask=1;
    j->prio=0;
    j->id=0;
    j->state=waiting;
}

void job_destroy(job *j)
{
    free(j->buf);
    free(j->argv);
    free(j->envp);
    free(j->stdout_filename);
    /*if (j->error_string)
        free(j->error_string);*/
}


void job_reinit(job *j)
{
    char *ret=strrchr(j->cmd, '/');

    /* set the job name */
    if (ret)
        j->name=ret+1;
    else
        j->name=j->cmd;
   
    /* set the stdout name */
    j->stdout_filename=malloc_check_server(sizeof(char)*MAXPATHLEN);
    snprintf(j->stdout_filename,MAXPATHLEN, "%s.%d.out", j->name, j->id);

    /* set the submit time */
    j->sub_time=time(NULL);
}

int job_gt(job *ja, job *jb)
{
    /* The sort order is: 
       state
       - run_order for job_state == running
       - prio for job_state == waiting, error
       submit time */

    if (ja->state > jb->state)
        return 1;
    else if (ja->state < jb->state)
        return 0;

    /* job states are the same */
    if (ja->state == running)
    {
        if (ja->run_order > jb->run_order)
            return 1;
        else if (ja->run_order < jb->run_order)
            return 1;
    }
    else
    {
        /* check for two cases of unequal priority */
        if (ja->prio > jb->prio)
            return 1;
        else if (ja->prio < jb->prio)
            return 0;
    }

    /* check for submission time */
    if (ja->sub_time != jb->sub_time)
    {
        return (ja->sub_time < jb->sub_time);
    }

    /* the lowest sort order */
    return ja->id < jb->id;
}

void job_run(job *j, int run_order)
{
    pid_t ret;
    int stdo=-1,stdi=-1;
    int rret;
    /*char stdout_filename[MAXPATHLEN];*/
    char *pathname=NULL;
    char pathname_str[MAXPATHLEN];
    const char *error_string;

    if (debug>0)
        printf("SERVER: RUNNING JOB %s\n", j->name);

    /* change directory. The parent will change back to / after fork() */
    rret=chdir(j->wd);
    if (rret != 0)
    {
        error_string="couldn't chdir to run directory";
        goto error;
    }

    /* open stdout and stdin (which is /dev/null) */
    /*snprintf(stdout_filename,MAXPATHLEN, "%s.%d.out", j->name, j->id);*/
    stdo=open(j->stdout_filename, O_WRONLY|O_CREAT|O_TRUNC, 
              S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (stdo<0)
    {
        error_string="couldn't open stdout";
        goto error;
    }
    stdi=open("/dev/null", O_RDONLY);
    if (stdi<0)
    {
        error_string="couldn't open /dev/null for stdin";
        goto error;
    }

    /* and now spawn the child process */
    ret=fork();
    if (ret<0)
    {
        if (errno!=EAGAIN)
        {
            j->state=run_error;
            fatal_server_system_error("fork");
        }
        else
        {
            return;
        }
    }
    else if (ret>0)
    {
        /* the parent. */
        if (chdir("/") < 0)
            server_system_error("chdir to / failed");
        j->pid=ret;
        j->state=started;
        j->run_order=run_order;
        close(stdi);
        close(stdo);

        if (debug>0)
            printf("SERVER: pid=%d\n", ret);

        j->start_time=time(NULL);
    }
    else
    {
        /* the child. */

        /* Become process group leader so me and my children may be killed
           in one fell swoop. There's nothing much we can do if this fails,
           so we don't check return values. */
        setpgid(0, getpid());

        /* take care of the stdin/out/err files */
        close(STDOUT_FILENO);
        dup2(stdo, STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(stdo, STDERR_FILENO);
        close(STDIN_FILENO);
        dup2(stdi, STDIN_FILENO);


        /* search PATH */
        /* find the path of the executable */
        if ( strchr(j->cmd, '/') )
        {
            /* if there's a slash in the command, we don't look in PATH */
            pathname=j->cmd;

            execve(pathname, j->argv, j->envp);
        }
        else
        {
            char *path=_PATH_DEFPATH;
            int i=0;
            /* find PATH in the environment variables */
            while(j->envp[i])
            {
                if (strstr(j->envp[i], "PATH=") == j->envp[i])
                {
                    path=j->envp[i]+strlen("PATH=");
                }
                i++;
            }

            while((*path)!=0)
            {
                char *end=strchr(path, ':');
                size_t len;
                if (!end)
                {
                    len=strlen(path);
                    end=path+len-1;
                }
                else
                {
                    len=end-path;
                }
                strncpy(pathname_str, path, len);
                pathname_str[len]='/';
                strncpy(pathname_str+len+1, j->cmd, MAXPATHLEN-len);
                execve(pathname_str, j->argv, j->envp);
                path=end+1;
            }
        }

        printf("Couldn't find %s\n", j->cmd);
        exit(1);
    }
    return;
error:
    j->error_string=(char*)malloc(ERRSTRING_LEN);
    snprintf(j->error_string, ERRSTRING_LEN, "%s: %s", error_string, 
             strerror(errno));
    j->state=run_error;
    if (stdi>0)
        close(stdi);
    if (stdo>0)
        close(stdo);
    {
        if (chdir("/") < 0)
            server_system_error("chdir to / failed");
    }
    if (debug>1)
        printf("SERVER: job %s ERROR: %s\n", j->name, j->error_string);
}

void job_cancel(job *j)
{
    if (debug>0)
        printf("CANCELING JOB %s\n", j->name);

    if (j->state == running)
    {
        killpg(j->pid, SIGTERM);
        /* we let the signal handler take care of the 
           cleanup here. */
    }
}



