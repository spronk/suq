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
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>



#include "log_err.h"
#include "srv_config.h"
#include "server.h"
#include "connection.h"
#include "job.h"
#include "sig_handler.h"

#define NSIGREAD 16

/* explicitly a global variable to allow signal handler access */
suq_serv cs;


#define fscount(n, fs)  n = ( (fs) > (n) ? (fs)  : (n) )

void suq_serv_main(suq_config *sc, int keep_alive, int pipe_in, int pipe_out)
{
    sig_handler sh;
    int stop_server;

    pdebug("Server started\n");

    suq_serv_init(&cs, sc, keep_alive, pipe_in, pipe_out);
    sig_handler_init(&sh);

    /* now check for incoming data. */
    do
    {
        fd_set rfs; /* the select file descriptor set */
        int retval; /* select() return value */
        int n=0; /* the highest-value fd */
        connection *cn;
        int shf=sig_handler_get_reader(&sh);

        FD_ZERO(&rfs);

        FD_SET(cs.sockdes, &rfs);
        fscount(n, cs.sockdes);
        pdebug("SERVER: Adding select fd %d from socket\n", cs.sockdes);

        FD_SET(shf, &rfs);
        fscount(n, shf);
        pdebug("SERVER: Adding select fd %d from pipe\n", shf);

        conn_list_remove_closed(&(cs.cl));
        cn=conn_list_first(&(cs.cl));
        while(cn)
        {
            FD_SET(cn->read_fd, &rfs);
            fscount(n, cn->read_fd);
            pdebug("SERVER: Adding select fd %d from conn\n", cn->read_fd);

            cn = conn_list_next(&(cs.cl), cn);
        }

        pdebug("SERVER: waiting for input on %d connections...\n",
               conn_list_N(&(cs.cl)));

        retval=0;
        retval=select(n+1, &rfs, NULL, NULL, NULL);

        pdebug("SERVER: select returned %d\n", retval);

        /* now check whether we can run new jobs */
        joblist_check_run(&(cs.jl), &cs);

        if (retval < 0)
        {
            if (errno!=EAGAIN && errno!=EINTR)
            {
                fatal_server_system_error("Select failed");
            }
        }
        else if (retval > 0)
        {
            if (FD_ISSET(shf, &rfs))
            {
                char dum[NSIGREAD];

                if (read(shf, dum, NSIGREAD) < 0)
                {
                    fatal_server_system_error("sig handler read failed");
                }
                suq_serv_wait_proc(&cs);
            }
            if (FD_ISSET(cs.sockdes, &rfs))
            {
                suq_serv_accept_connection(&cs);
            }
            cn=cs.cl.head->next;
            cn = conn_list_first(&(cs.cl));
            while(cn)
            {
                /* we remember this beforehand, because the functions here
                   might remove the connection from the list */
                connection *next=conn_list_next(&(cs.cl), cn);
                if (FD_ISSET(cn->read_fd, &rfs))
                {
                    if (connection_read(cn, &cs)==0)
                    {
                        cn->read_open=0;
                        if (cn->keep_alive)
                        {
                            /* check whether it was a wait */
                            jobwait *jw=joblist_wait_search_conn(&(cs.jl), cn);
                            if (jw)
                            {
                                joblist_wait_remove(&(cs.jl), jw);
                            }
                        }
                        conn_list_remove(&(cs.cl), cn);
                        cn=NULL;
                    }
                    if (cn && !cn->keep_alive)
                    {
                        conn_list_remove(&(cs.cl), cn);
                    }
                }
                cn = next;
            }
        }
        joblist_wait_check_finished_all( &(cs.jl), &(cs.cl) );

        stop_server = (conn_list_N(&(cs.cl)) < 1) && (joblist_N(&(cs.jl)) < 1);

        if (stop_server && cs.server_keepalive)
        {
            pdebug("SERVER: Not quitting because server must be kept alive\n");
            stop_server = 0;
        }
    } while (!stop_server);

    pdebug("Server stopped");

    sig_handler_destroy(&sh);
    suq_serv_destroy(&cs);
}



void suq_serv_init(suq_serv *cs, suq_config *sc,
                   int keep_alive, int pipe_in, int pipe_out)
{
    int optval=1;
    struct sockaddr_un server_addr;
    char *log_file_name;

    cs->sc = sc;
    cs->server_keepalive = keep_alive;

    joblist_init(&(cs->jl)); /* create an empty job list */
    conn_list_init(&(cs->cl)); /* and a new connection list */

    /* we chdir to / */
    if (chdir("/") < 0)
        server_system_error("chdir to / failed");

    /* open AF_UNIX socket */
    cs->sockdes = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cs->sockdes < 0)
        fatal_server_system_error("listening socket failed");

    if (setsockopt(cs->sockdes, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval))<0)
        fatal_server_system_error("socket options");

    /* we remove the socket file but don't check the results*/
    unlink(cs->sc->socket_filename);

    memset(&server_addr,0,sizeof(server_addr)); /* reset values */
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, cs->sc->socket_filename,
            sizeof(server_addr.sun_path));

    /* bind */
    if (bind(cs->sockdes, (const struct sockaddr*)&server_addr,
             SUN_LEN(&server_addr)) < 0)
        fatal_server_system_error("server bind failed");

    /* set permissions */
    chmod(cs->sc->socket_filename, S_IRUSR|S_IWUSR);

    /* and listen */
    if (listen(cs->sockdes, 10)<0)
        fatal_server_system_error("server listen failed");


    pdebug("SERVER: initialized on %s\n", cs->sc->socket_filename);


    if (pipe_in>0 && pipe_out>0)
        conn_list_add(&(cs->cl), connection_new(pipe_in, pipe_out));

    /* we now have a bound socket that we can listen() to */

    log_file_name = suq_config_get_job_log_name(cs->sc);
    cs->job_log_file = fopen(log_file_name, "w");
    if (!cs->job_log_file)
        fatal_server_system_error("Error opening job log file %s",
                                  log_file_name);
    free(log_file_name);
}


void suq_serv_destroy(suq_serv *cs)
{
    /* we just unlink the file */
    unlink(cs->sc->socket_filename);

    fclose(cs->job_log_file);

    conn_list_destroy(&(cs->cl));
    joblist_destroy(&(cs->jl));
}

void suq_serv_accept_connection(suq_serv *cs)
{
    /* accept incoming connection */
    int nfd=accept(cs->sockdes, NULL, NULL);
    if (nfd < 0)
        fatal_server_system_error("server connection accept failed");

    pdebug("SERVER: accepted connection\n");

    conn_list_add(&(cs->cl), connection_new(nfd, nfd));
}

void suq_serv_wait_proc(suq_serv *cs)
{
    pid_t ret;
    int status;

    do
    {
        ret=wait4(-1, &status, WNOHANG , NULL);

        if (ret>0)
        {
            /* find the job */
            job *j=joblist_first( &(cs->jl) );
            job *j_found=NULL;

            /* a process really quit */
            pdebug("SERVER: CHILD pid=%d CAUGHT\n", ret);

            while (j)
            {
                job *next=joblist_next( &(cs->jl), j);

                if ( (j->state==running || j->state==started) &&
                     (j->pid == ret))
                {
                    j_found = j;
                    job_finish(j, status);
                    break;
                }
                j=next;
            }
            if (!j_found)
            {
                pdebug("SERVER: ERROR child not found\n");
            }
        }
    } while (ret>0);

    joblist_check_run(&(cs->jl), cs);
}

