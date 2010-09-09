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


#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#define CONN_BUF_SIZE 10*1024

typedef struct connection
{
    int read_fd, write_fd; /* the file descriptors */

    struct connection *prev,*next; /* conns are in a circular list */

    char *read_buf; /* read buffer. */
    size_t read_buf_alloc; /* allocated size of read_buf */
    size_t read_buf_cursor; /* end of last read in read_buf */
    size_t read_buf_request; /* end of the request in read_buf */
    
    int read_open; /* whether the connection is still open for reads */
    int write_open; /* whether the connection is still open for writes */

    int keep_alive; /* whether the connection should be kept alive after
                       the read EOFs */
} connection;

/* the circular list header */
typedef struct conn_list
{
    connection *head; /* the dummy head element */
    int N; /* the number of elems */

    connection head_elem; /* the pre-allocated head element list */
} conn_list;


/* initialize the connection list */
void conn_list_init(conn_list *cl);


/* destroy the connection list contents */
void conn_list_destroy(conn_list *cl);



/* add a connection to the list  */
void conn_list_add(conn_list *cl, connection *c);

/* remove a connection from the list & close and destroy the associated
    connection. */
void conn_list_remove(conn_list *cl, connection *c);

/* remove all closed connections from the list */
void conn_list_remove_closed(conn_list *cl);

/* get the number of elems */
int conn_list_N(conn_list *cl);

/* get the first connection, or NULL if there's no connections */
connection *conn_list_first(conn_list *cl);
/* get the next connection, or NULL when there's no connections left */
connection *conn_list_next(conn_list *cl, connection *c);




/* initialize a connection */
connection *connection_new(int read_fd, int write_fd);

/* close the write connection */
void connection_close(connection *cn);

/* close & deallocate the contents of the connection */
void connection_destroy(connection *cn);

struct suq_serv;
/* read data, and process it */
int connection_read(connection *conn, struct suq_serv *cs);

#endif

