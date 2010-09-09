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
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include "err.h"


int debug=0;

void fatal_system_error(const char *message)
{
    fprintf(stderr, "ERROR: %s : %s\n", message, strerror(errno));
    exit(1);
}

void fatal_error(const char *message)
{
    fprintf(stderr, "ERROR: %s : %s\n", message, strerror(errno));
    exit(1);
}



void fatal_server_system_error(const char *message)
{
    server_system_error(message);
    exit(1);
}

void fatal_server_error(const char *message)
{
    server_error(message);
    exit(1);
}

void server_system_error(const char *message)
{
    fprintf(stderr, "SERVER ERROR: %s : %s\n", message, strerror(errno));
}

void server_error(const char *message)
{
    fprintf(stderr, "SERVER ERROR: %s : %s\n", message, strerror(errno));
}






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


