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
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>


#include "err.h"
#include "settings.h"



/* get the file name of the socket file */
static char *get_sockname(void);


/* get the file name of the log file */
static char *get_log_filename(const char *full_dirname, const char *hostname);
/* get the file name of the settings file */
static char *get_settings_filename(const char *full_dirname, const char *hostname);
/* get the file name of the settings file */
static char *get_settings_tmpname(const char *settings_filename);





char *get_sockname(void)
{
    char *sockname;
    uid_t uid;
    int ret;

    sockname=malloc_check_server(sizeof(char)*MAXPATHLEN);

    /*pw=getpwuid(getuid());*/
    uid=getuid();

    /* construct the directory name relative to /tmp */
    snprintf(sockname, MAXPATHLEN, "/tmp/.suq.%d", uid);

    /* first make the directory */
    ret=mkdir(sockname, S_IRWXU);
    if ( (ret < 0) && (errno!=EEXIST) )
        fatal_system_error("Can't create socket directory");

    /* now set the actual socket name */
    snprintf(sockname, MAXPATHLEN, "/tmp/.suq.%d/suq.sock", uid);

    return sockname;
}

char *get_log_filename(const char *full_dirname, const char *hostname)
{
    char *filename;

    filename=malloc_check_server(sizeof(char)*MAXPATHLEN);
    /* construct the name relative to home */
    snprintf(filename, MAXPATHLEN, "%s/%s.log", full_dirname, hostname);

    return filename;
}

char *get_settings_filename(const char *full_dirname, const char *hostname)
{
    char *filename;

    filename=malloc_check_server(sizeof(char)*MAXPATHLEN);
    /* construct the name relative to home */
    snprintf(filename, MAXPATHLEN, "%s/%s.conf", full_dirname, hostname);

    return filename;
}

char *get_settings_tmpname(const char *settings_filename)
{
    char *filename;

    filename=malloc_check_server(sizeof(char)*MAXPATHLEN);
    /* construct the name relative to home */
    snprintf(filename, MAXPATHLEN, "%s.tmp", settings_filename);

    return filename;
}



void suq_settings_init(suq_settings *st, const char *dirname)
{
    char hostname[_POSIX_HOST_NAME_MAX+1];

    if (!dirname)
    {
        dirname=getenv("HOME");
    }
    st->dirname=strdup(dirname);

    /* now get the full directory name */
    st->fulldirname=malloc(sizeof(char)*MAXPATHLEN);
    snprintf(st->fulldirname, MAXPATHLEN, "%s/.suq", st->dirname);
    if (debug>1)
        printf("CLIENT: fulldirname='%s'\n", st->fulldirname);


    /* set the default settings */
#if defined(_SC_NPROCESSORS_ONLN)
    st->ntask=sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    st->ntask=sysconf(_SC_NPROC_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
    st->ntask=sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF)
    st->ntask=sysconf(_SC_NPROC_CONF);
#else
    st->ntask=1;
#endif
    st->next_id=0;
    gethostname(hostname, _POSIX_HOST_NAME_MAX);
    st->settings_filename=get_settings_filename(st->fulldirname, hostname);
    st->sock_filename=get_sockname();
    st->log_filename=get_log_filename(st->fulldirname, hostname);

    st->settings_tmpname=get_settings_tmpname(st->settings_filename);

    /* for later operations, we need to have the directory in place, so
       we make sure of it. */
    {
        int ret;
        struct stat statbuf;


        /* we don't want anybody to have access to this directory - 
           otherwise they might submit jobs here. */
        mkdir(st->fulldirname, S_IRWXU);
        /* we don't check for mkdir()'s error, but just stat for
            the presence of a directory */
        ret=stat(st->fulldirname, &statbuf);
        if (ret!=0)
        {
            fatal_system_error("Base directory not accessible");
        }
        if (! ( (statbuf.st_mode & S_IFDIR) ) )
        {
            fatal_system_error("Base directory not a directory");
        }
        if  (access(st->fulldirname, R_OK|W_OK|X_OK) != 0)
        {
            fatal_system_error("No permissions to base directory ");
        }
    }
}

void suq_settings_destroy(suq_settings *st)
{
    free(st->dirname);
    free(st->fulldirname);
    free(st->settings_filename);
    free(st->settings_tmpname);
    free(st->sock_filename);
    free(st->log_filename);
}


/* for now we just use stdio functions for reading configuration files */
void suq_settings_read(suq_settings *st)
{
#define READLEN 255
    FILE *in;
    int ret;
    char name[READLEN], val[READLEN];
    int valn;
    char *end;

    in=fopen(st->settings_filename, "r");
    if (!in)
        return;
    do
    {
        ret=fscanf(in, "%255s = %255s\n",  name, val );
        if (ret>0)
        {
            if (debug>1)
                printf("name='%s', val='%s'\n", name, val);

            if (strcmp(name, "ntask")==0)
            {
                valn=strtol(val, &end, 0);
                if (val!=end)
                    st->ntask=valn;
            }
#ifdef SUQ_SETTINGS_NEXT_ID
            else if (strcmp(name, "next_id")==0)
            {
                valn=strtol(val, &end, 0);
                if (val!=end)
                    st->next_id=valn;
            }
#endif
        }
    }
    while (ret>0);
    fclose(in);
}


void suq_settings_write(suq_settings *st)
{
    FILE *out;

    out=fopen(st->settings_tmpname, "w");
    if (!out)
        fatal_server_system_error("open: Writing server settings");

    if (fprintf(out, "ntask = %d\n", st->ntask) < 0)
        fatal_server_system_error("write: Writing server settings");

#ifdef SUQ_SETTINGS_NEXT_ID
    if (fprintf(out, "next_id = %d\n", st->next_id) < 0)
        fatal_server_system_error("write: Writing server settings");
#endif

    if (fclose(out))
        fatal_server_system_error("close: Writing server settings");

    rename(st->settings_tmpname, st->settings_filename);
}


void suq_settings_set_ntask(suq_settings *st, int ntask)
{
    st->ntask=ntask;
    suq_settings_write(st);
}


int suq_settings_get_next_id(suq_settings *st)
{
    int ret=++(st->next_id);
    if (ret > 10000)
    {
        ret=st->next_id=0;
    }
#ifdef SUQ_SETTINGS_NEXT_ID
    suq_settings_write(st);
#endif
    return ret;
}


