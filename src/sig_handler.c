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
#include <errno.h>
#include <signal.h>
#include <fcntl.h>



#include "log_err.h"
#include "srv_config.h"
#include "server.h"
#include "connection.h"
#include "job.h"
#include "sig_handler.h"

/* the local version that's accessible from the signal handler */
static int sig2main;

/* the actual signal handler */
static void sig_handler_handle(int sig, siginfo_t *info, void *uap);

void sig_handler_init(sig_handler *sh)
{
    struct sigaction sa;
    sigset_t set;
    int pipes[2];


    if (pipe(pipes) < 0)
    {
        fatal_server_system_error("Signal handler install failed (pipe)");
    }

    sh->sig2main = sig2main = pipes[1];
    sh->main2sig = pipes[0];

    fcntl(sh->sig2main, O_NONBLOCK);
    /* now set the close-on-exec flag because we don't want children to
       inherit these. */
    fcntl(sh->sig2main, F_SETFD, 1);
    fcntl(sh->sig2main, F_SETFD, 1);

#if 0
    if (fcntl(sh->sig2main, O_NONBLOCK) < 0)
    {
        fatal_server_system_error("Signal handler install failed (fcntl)");
    }
#endif

    /* now attach signal handlers: */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);

    sa.sa_sigaction = sig_handler_handle;
    sa.sa_mask=set;
    sa.sa_flags=SA_NOCLDSTOP;

    if ( (sigaction(SIGCHLD, &sa, NULL ) < 0) ||
         (sigaction(SIGUSR1, &sa, NULL ) < 0) )
    {
        fatal_server_system_error("Signal handler install failed (sigaction)");
    }
}

int sig_handler_get_reader(sig_handler *sh)
{
    return sh->main2sig;
}


void sig_handler_destroy(sig_handler *sh)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, NULL);

    close(sh->sig2main);
    close(sh->main2sig);
}

#if 0
void sig_handler_block(sig_handler *sh)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void sig_handler_unblock(sig_handler *sh)
{
}
#endif

void sig_handler_handle(int sig, siginfo_t *info, void *uap)
{
    char dum='s';

    /* write a byte in non-blocking mode. This will be picked up
       by the server's select() loop  */
    if (write(sig2main, &dum, 1) < 0)
        fatal_server_system_error("sig_handler write failed");
}


