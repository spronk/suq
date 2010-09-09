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


#ifndef __SETTINGS_H__
#define __SETTINGS_H__

/* uncomment this to remember the next job id */
/*#define SUQ_SETTINGS_NEXT_ID */

/* the client + server settings. These are read from the basedir that's 
    specified on the command line of the client, and passed on to the server
    once it is spawend. */
typedef struct suq_settings
{
    int ntask; /* max number of processors to run on */
    unsigned int next_id; /* next job id */

    char *dirname; /* the base directory */
    char *fulldirname; /* the suq directory */
    char *settings_filename; /* settings file name */
    char *settings_tmpname; /* temporary settings file name for writing*/
    char *sock_filename; /* socket file name */
    char *log_filename; /* log file name */
} suq_settings;


/* initialize suq_settings, and set the values  to defaults, 
   from base directory name dirname */
void suq_settings_init(suq_settings *st, const char *dirname);



/* read suq_settings, */
void suq_settings_read(suq_settings *st);

/* deallocate settings */
void suq_settings_destroy(suq_settings *st);

/* write suq settings */
void suq_settings_write(suq_settings *st);


/* set the max. number of tasks */
void suq_settings_set_ntask(suq_settings *st, int ntask);
/* get the next job ID */
int suq_settings_get_next_id(suq_settings *st);

#endif
