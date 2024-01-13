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



#ifndef __CLIENT_CONN_H__
#define __CLIENT_CONN_H__

#define CLIENT_CONN_BUF_SIZE 10*1024

typedef struct
{
    int fd_read; /* input file descriptor */
    int fd_write; /* output file descriptor */
} client_connection;

/* try to connect to an already existing daemon, or spawn a new one */
int client_try_connection(client_connection *cc, suq_config *sc);

/* connect to an already existing daemon, or spawn a new one. */
void client_connection_init(client_connection *cc, suq_config *sc);
/* close a daemon connection */
void client_connection_destroy(client_connection *dc);

/* send a request in the form of argc, argv. Can only be done once
    per connection.*/
void client_connection_send_request(client_connection *dc, int argc,
                                    char *argv[]);

/* get the results as a string from a daemon, with an error code.
    Runs until incoming connection closes. */
void client_connection_get_print_results(client_connection *dc, int *errcode);

#endif

