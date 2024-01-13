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
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include "log_err.h"


int debug = 0;

/* implementation of all timestamp-prints */
void vfprintf_tm_log(time_t tm, const char *format, va_list ap)
{
    char timestr[CTIME_R_STRLEN + 1];
    char *loc;

    ctime_r(&tm, timestr);
    loc = strchr(timestr, '\n'); /* remove newline */
    if (loc)
        *loc = 0;

    printf("%s: ", timestr);
    vfprintf(stdout, format, ap);
    if (strchr(format, '\n') == NULL)
    {
        printf("\n");
    }
    fflush(stdout);
}

void print_tm_log(time_t tm, const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    vfprintf_tm_log(tm, format, argptr);
    va_end(argptr);
}

void print_log(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    vfprintf_tm_log(time(NULL), format, argptr);
    va_end(argptr);
}

void pdebug(const char *format, ...)
{
    if (debug)
    {
        va_list argptr;

        va_start(argptr, format);
        vfprintf_tm_log(time(NULL), format, argptr);
        va_end(argptr);
    }
}



/* error functions */

/* basic printing*/
static void err_msg(const char *prefix, const char *format, va_list argptr)
{

    fprintf(stderr, "%s: ", prefix);
    vfprintf(stderr, format, argptr);
}

static void err_msg_errno(const char *prefix, const char *format,
                          va_list argptr)
{
    err_msg(prefix, format, argptr);
    fprintf(stderr, " : %s \n", strerror(errno));
}

/* client errors */
void fatal_error(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    err_msg("ERROR", format, argptr);
    va_end(argptr);
    exit(1);
}

void fatal_system_error(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    err_msg_errno("ERROR", format, argptr);
    va_end(argptr);
    exit(1);
}


/* server errors */
void server_error(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    err_msg("SERVER ERROR", format, argptr);
    va_end(argptr);
}

void server_system_error(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    err_msg_errno("SERVER ERROR", format, argptr);
    va_end(argptr);
}


void fatal_server_error(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    server_error(format, argptr);
    va_end(argptr);
    exit(1);
}

void fatal_server_system_error(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    server_system_error(format, argptr);
    va_end(argptr);
    exit(1);
}


/* checked allocations */
void *malloc_check(size_t sz)
{
    void* ret=malloc(sz);
    if (!ret)
        fatal_system_error("Out of memory");
    return ret;
}

void *realloc_check(void *ptr,size_t sz)
{
    void* ret=realloc(ptr, sz);
    if (!ret)
        fatal_system_error("Out of memory");
    return ret;
}

void *malloc_check_server(size_t sz)
{
    void* ret=malloc(sz);
    if (!ret)
        fatal_server_system_error("Out of memory");
    return ret;
}

void *realloc_check_server(void *ptr,size_t sz)
{
    void* ret=realloc(ptr, sz);
    if (!ret)
        fatal_server_system_error("Out of memory");
    return ret;
}

void ignore_error(int return_code)
{
}
