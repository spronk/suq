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
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>


#include "log_err.h"
#include "srv_config.h"


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX MAXHOSTNAMELEN
#endif


/* make a single directory if it doesn't already exist */
void makedir(const char *path, mode_t mode)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        /* Directory doesn't exist, EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            fatal_system_error("Can't create directory %s", path);
    }
    else if (!S_ISDIR(st.st_mode))
    {
        fatal_system_error("Can't create directory: %s is not a directory",
                           path);
    }
}

void makedirs(const char *path, mode_t mode)
{
    char *foundp, *searchp, *lastfoundp=NULL;
    int status;
    /* cop so we can write \0 into it */
    char *newpath = strdup(path);

    status = 0;
    searchp = newpath;
    while (status == 0 && (foundp = strchr(searchp, '/')) != 0)
    {
        /* not a root slash or a double slash */
        if (foundp != searchp)
        {
            /* terminate path here, temporarily */
            *foundp = '\0';
            makedir(newpath, mode);
            *foundp = '/';
        }
        lastfoundp = foundp;
        searchp = foundp + 1;
    }

    /* now check the last found directory's permissions */
    if (lastfoundp != NULL)
    {
        int ret;
        struct stat statbuf;

        /* we didn't check for mkdir()'s error, but just stat for presence
           of the directory */
        *lastfoundp = '\0';
        ret = stat(newpath, &statbuf);
        if (ret != 0)
        {
            fatal_system_error("Directory %s not accesible", newpath);

        }
        if (!((statbuf.st_mode & S_IFDIR)))
        {
            fatal_system_error("Directory %s not a directory", newpath);

        }
        if (access(newpath, R_OK | W_OK | X_OK) != 0)
        {
            fatal_system_error("Insufficient permissions to directory %s",
                               newpath);
        }
        *lastfoundp = '/';
    }
    free(newpath);
}


/*
    Path functions
*/

/* construct a file/directory name relative to an optional environment
   variable. If the env var does not exist, use homedir and
   default_rel_to_home to construct a default value.

   output goes to a pre-allocated string out, with maximum length maxlen
   (should be MAXPATHLEN + 1) */
void get_env_dir(char *out,
                 size_t maxlen,
                 const char *env_name,
                 const char *homedir,
                 const char *default_rel_to_home)
{
    char *envv_name = getenv(env_name);

    if (envv_name == NULL)
    {
        if (homedir == NULL)
        {
            errno = ENOENT;
            fatal_system_error("No home directory found");
        }
        snprintf(out, maxlen-1, "%s/%s", homedir, default_rel_to_home);
    }
    else
    {
        strncpy(out, env_name, maxlen-1);
    }
}




/* get the file name of the socket file */
char *get_sockname(const char *hostname)
{
    char *sockname;
    uid_t uid = getuid();

    sockname=malloc_check_server(sizeof(char)*(MAXPATHLEN + 1));
    snprintf(sockname, MAXPATHLEN, "/tmp/suq-%d/%s.socket", uid, hostname);

    return sockname;
}

static char *get_log_dir(const char *homedir)
{
    char *log_dir_out;
    char ldir[MAXPATHLEN + 1];

    log_dir_out = malloc_check_server(sizeof(char) * (MAXPATHLEN + 1));

    get_env_dir(ldir, MAXPATHLEN, "XDG_STATE_HOME", homedir, ".local/state");
    snprintf(log_dir_out, MAXPATHLEN, "%s/suq/", ldir);
    return log_dir_out;
}

static char *get_output_dir(void)
{
    uid_t uid = getuid();
    char *out_dir;

    out_dir = malloc_check_server(sizeof(char) * (MAXPATHLEN + 1));
    snprintf(out_dir, MAXPATHLEN, "/tmp/suq-%d/", uid);

    return out_dir;
}

char *suq_config_get_filename(const char *given_filename)
{
    char config_home[MAXPATHLEN + 1];
    char hostname[HOST_NAME_MAX + 1];
    char *homedir;
    char *config_name;

    if (given_filename != NULL)
    {
        return strdup(given_filename);
    }

    homedir = getenv("HOME");
    if (homedir == NULL)
    {
        fprintf(stderr, "ERROR: no home directory.\n");
        fprintf(stderr, "Specify the configuration file as parameter.\n");
        exit(EXIT_FAILURE);
    }

    gethostname(hostname, HOST_NAME_MAX);
    get_env_dir(config_home, MAXPATHLEN, "XDG_CONFIG_HOME", homedir, ".config");

    config_name = malloc_check_server(sizeof(char) * (MAXPATHLEN + 1));
    snprintf(config_name, MAXPATHLEN, "%s/suq/%s.conf", config_home, hostname);

    return config_name;
}

static char *get_config_tmpname(const char *config_filename)
{
    char *filename;

    filename = malloc_check_server(sizeof(char) * (MAXPATHLEN + 1));
    /* append to the config filename */
    snprintf(filename, MAXPATHLEN, "%s.tmp", config_filename);

    return filename;
}


void suq_config_init(suq_config *sc, const char *filename)
{
/* MAXPATHLEN is at least 256 in POSIX */
#define READLEN MAXPATHLEN + 1
    FILE *in;
    int ret;
    char name[READLEN], val[READLEN];
    int valn;
    char *end;
    char *homedir;

    /* set the default settings */
#if defined(_SC_NPROCESSORS_ONLN)
    sc->ntask=sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    sc->ntask=sysconf(_SC_NPROC_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
    sc->ntask=sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF)
    sc->ntask=sysconf(_SC_NPROC_CONF);
#else
    sc->ntask=1;
#endif
    sc->next_id=0;
    sc->config_filename = strdup(filename);

    sc->socket_filename = NULL;
    sc->log_dir = NULL;
    sc->output_dir = NULL;
    sc->dirty = 0;

    makedirs(filename, S_IRWXU);

    sc->hostname = malloc_check_server(sizeof(char) * (HOST_NAME_MAX + 1));
    gethostname(sc->hostname, HOST_NAME_MAX);

    pdebug("Reading server configuration from %s", filename);

    /* read the file. It's not an error if it doesn't work out because the
       file might not be there */
    in=fopen(filename, "r");
    if (in)
    {
        do
        {
            ret=fscanf(in, "%255s = %255s\n",  name, val);
            if (ret>0)
            {
                pdebug("name='%s', val='%s'\n", name, val);

                if (strcmp(name, "ntask")==0)
                {
                    valn=strtol(val, &end, 0);
                    if (val!=end)
                        sc->ntask=valn;
                }
                else if (strcmp(name, "next_id")==0)
                {
                    valn=strtol(val, &end, 0);
                    if (val!=end)
                        sc->next_id=valn;
                }
                else if (strcmp(name, "socket_filename") == 0)
                {
                    sc->socket_filename = strndup(val, MAXPATHLEN);
                }
                else if (strcmp(name, "log_dir") == 0)
                {
                    sc->log_dir = strndup(val, MAXPATHLEN);
                }
                else if (strcmp(name, "output_dir") == 0)
                {
                    sc->output_dir = strndup(val, MAXPATHLEN);
                }
            }
        }
        while (ret>0);
        fclose(in);
    }

    /* now fill any missing path vars */
    homedir = getenv("HOME");

    if (sc->socket_filename == NULL)
    {
        sc->socket_filename = get_sockname(sc->hostname);
    }
    if (sc->log_dir == NULL)
    {
        sc->log_dir = get_log_dir(homedir);
    }
    if (sc->output_dir == NULL)
    {
        sc->output_dir = get_output_dir();
    }
}

void suq_config_write(suq_config *sc)
{
    FILE *out;
    char *tmpname = get_config_tmpname(sc->config_filename);

    pdebug("Saving conf to %s", sc->config_filename);

    out=fopen(tmpname, "w");
    if (!out)
        fatal_server_system_error("open: Writing server settings to %s",
                                  tmpname);

    if (fprintf(out, "ntask = %d\n", sc->ntask) < 0)
        fatal_server_system_error("write: Writing server settings");

    if (fprintf(out, "next_id = %d\n", sc->next_id) < 0)
        fatal_server_system_error("write: Writing server settings");

    if (fprintf(out, "socket_filename = %s\n", sc->socket_filename) < 0)
        fatal_server_system_error("write: Writing server settings");

    if (fprintf(out, "log_dir = %s\n", sc->log_dir) < 0)
        fatal_server_system_error("write: Writing server settings");

    if (fprintf(out, "output_dir = %s\n", sc->output_dir) < 0)
        fatal_server_system_error("write: Writing server settings");

    if (fclose(out))
        fatal_server_system_error("close: Writing server settings");

    sc->dirty = 0;

    if (rename(tmpname, sc->config_filename))
    {
        fatal_server_system_error("Renaming %s to %s",
                                  tmpname,
                                  sc->config_filename);
    }

    free(tmpname);
}

void suq_config_makedirs(const suq_config *sc)
{
    makedirs(sc->socket_filename, S_IRWXU);
    makedirs(sc->log_dir, S_IRWXU);
    makedirs(sc->output_dir, S_IRWXU);
}


char *suq_config_get_server_log_name(const suq_config *sc)
{
    char *log_name;

    log_name = malloc_check_server(sizeof(char) * (MAXPATHLEN + 1));
    snprintf(log_name, MAXPATHLEN, "%s/%s.server.log", sc->log_dir,
             sc->hostname);

    return log_name;
}

char *suq_config_get_job_log_name(const suq_config *sc)
{
    char *log_name;

    log_name = malloc_check_server(sizeof(char) * (MAXPATHLEN + 1));
    snprintf(log_name, MAXPATHLEN, "%s/%s.job.log", sc->log_dir,
             sc->hostname);

    return log_name;
}

void suq_config_set_ntask(suq_config *sc, int ntask)
{
    sc->ntask=ntask;
    suq_config_write(sc);
}

int suq_config_get_next_id(suq_config *sc)
{
    int ret = ++(sc->next_id);
    if (ret > 10000)
    {
        ret = sc->next_id = 0;
    }
    sc->dirty = 1;

    return ret;
}

void suq_config_destroy(suq_config *sc)
{
    if (sc->dirty)
    {
        suq_config_write(sc);
    }

    free(sc->socket_filename);
    free(sc->log_dir);
    free(sc->output_dir);
    free(sc->hostname);
    free(sc->config_filename);
}

