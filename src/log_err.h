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


#ifndef __LOG_ERR_H__
#define __LOG_ERR_H__

#include <time.h>


#define DEBUG 2

/** whether, and to what level, to debug */
extern int debug;

/* Logging functions */

/* size of a time string */
#define CTIME_R_STRLEN 26
#define ISO_TIME_STRLEN 21

/** print to stdout with date/time stamp (effectively writing to log)
  *
  * @param tm       The timestamp to write
  * @param format   printf format string.
  */
void print_tm_log(time_t tm, const char *format, ...);

/** print to stdout with current date/time (effectively writing to log)
  *
  * @param format   printf format string.
  */
void print_log(const char *format, ...);

/** log with current timestamp to stdout conditional on debug > 1
  *
  * @param format   printf format string.
  */
void pdebug(const char *format, ...);




/* client error functions */

/** Print an error mesage and exit with error code.
 *
 * @param message The error message
*/
void fatal_error(const char *message);
/** Print an error message together with a message based on errno, and exit
 * with an error code.
 *
 * @param message The error message
*/
void fatal_system_error(const char *message);

/* server error functions */
/** Print an error message on the server side
 *
 * @param message The error message
*/
void server_error(const char *message);

/** Print an error message on the server side with a message based on an errno
 *
 * @param message The error message
*/
void server_system_error(const char *message);

/** Print an error message on the server side and exit
 *
 * @param message The error message
*/
void fatal_server_error(const char *message);

/** Print an error message on the server side with a message based on an errno,
 *  and exit
 *
 * @param message   The error message
*/
void fatal_server_system_error(const char *message);

/** checking malloc, for client and server
 *
 * @param sz    The size in bytes to allocate
*/
void *malloc_check(size_t sz);

/** checkking realloc, for client and server
 *
 * @param ptr       The pointer to reallocate
 * @param sz        The size to reallocate to
*/
void *realloc_check(void *ptr, size_t sz);

void *malloc_check_server(size_t sz);
void *realloc_check_server(void *ptr, size_t sz);

/** a convenience function to indicate that we're ignoring any errors
    associated with a system call that returns an int */
void ignore_error(int return_code);

#endif

