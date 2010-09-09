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


#ifndef __ERR_H__
#define __ERR_H__

#define DEBUG 2

/* controls whether to debug */
extern int debug; 

/* client error functions */
void fatal_error(const char *message);
void fatal_system_error(const char *message);

/* server error functions */
void server_error(const char *message);
void server_system_error(const char *message);
void fatal_server_error(const char *message);
void fatal_server_system_error(const char *message);

/* checking mallocs, for client and server */
void *malloc_check(size_t sz);
void *realloc_check(void *ptr, size_t sz);

void *malloc_check_server(size_t sz);
void *realloc_check_server(void *ptr, size_t sz);

/* a convenience function to indicate that we're ignoring any errors
   associated with a system call that returns an int */
void ignore_error(int return_code);

#endif

