/* This source code is part of

suq, the Single-User Queur

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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "log_err.h"
#include "connection.h"
#include "srv_config.h"
#include "server.h"
#include "request.h"

void conn_list_init(conn_list *cl)
{
    cl->head= &(cl->head_elem);
    cl->head->next=cl->head; /* the head element initially just points back */
    cl->head->prev=cl->head; /* the head element initially just points back */
    cl->N=0;
}

void conn_list_destroy(conn_list *cl)
{
    connection *c=conn_list_first(cl);

    while(c)
    {
        connection *next=conn_list_next(cl, c);
        connection_destroy(c);
        conn_list_remove(cl, c);
        c=next;
    }
}


connection *conn_list_first(conn_list *cl)
{
    connection *ret=cl->head->next;
    if (ret == cl->head)
        ret=NULL;
    return ret;
}


connection *conn_list_next(conn_list *cl, connection *c)
{
    connection *ret=c->next;
    if (ret == cl->head)
        ret=NULL;
    return ret;
}


void conn_list_add(conn_list *cl, connection *c)
{
    c->prev=cl->head;
    c->next=cl->head->next;

    c->next->prev=c;
    c->prev->next=c;
    cl->N++;
}

void conn_list_remove(conn_list *cl, connection *c)
{
    c->next->prev=c->prev;
    c->prev->next=c->next;

    /* just to make sure */
    c->next=NULL;
    c->prev=NULL;
    cl->N--;

    connection_close(c);
    connection_destroy(c);
    free(c);
}

int conn_list_N(conn_list *cl)
{
    return cl->N;
}

void conn_list_remove_closed(conn_list *cl)
{
    connection *cn=cl->head->next;
    while(cn != cl->head)
    {
        connection *next=cn->next;
        if (!cn->write_open || ((!cn->read_open) && (!cn->keep_alive) ) )
        {
            if (debug>1)
                printf("SERVER: removing connection\n");
            conn_list_remove(cl, cn);
        }
        cn = next;
    }
}

connection *connection_new(int read_fd, int write_fd)
{
    connection *cn=malloc_check_server(sizeof(connection));

    cn->read_fd=read_fd;
    cn->write_fd=write_fd;
    cn->prev=cn->next=NULL;

    cn->read_buf=NULL;
    cn->read_buf_alloc=0;
    cn->read_buf_cursor=0;
    cn->read_buf_request=0;

    cn->write_open=1;
    cn->read_open=1;
    cn->keep_alive=1;

    /* now set the close-on-exec flag because we don't want children to
       inherit these. */
    fcntl(read_fd, F_SETFD, 1);
    if (write_fd != read_fd)
        fcntl(write_fd, F_SETFD, 1);

    return cn;
}

void connection_close(connection *c)
{
    if ( (c->read_fd != c->write_fd) && (c->read_open) )
    {
        if (debug>1)
            printf("SERVER: closing %d\n", c->read_fd);
        close(c->read_fd);
        c->read_open=0;
    }
    if (c->write_open)
    {
        if (debug>1)
            printf("SERVER: closing %d\n", c->write_fd);
        close(c->write_fd);
        c->write_open=0;
    }
}

void connection_destroy(connection *c)
{
    connection_close(c);
    free(c->read_buf);
    c->read_fd=-1;
    c->write_fd=-1;
}

int connection_read(connection *c, suq_serv *cs)
{
    int res;
    size_t j;
    int end_found=0;

    /* first check the buffer, and reallocate if neccesary */
    if (c->read_buf_cursor >= c->read_buf_alloc)
    {
        int new_alloc = c->read_buf_alloc+CONN_BUF_SIZE;
        c->read_buf = realloc_check_server(c->read_buf, new_alloc);
        c->read_buf_alloc = new_alloc;
    }

    /* do the read we should do */
    res=read(c->read_fd,
             c->read_buf + c->read_buf_cursor,
             c->read_buf_alloc - c->read_buf_cursor);
    if (res<0)
    {
        server_system_error("Read error");
        return res;
    }
    c->read_buf_cursor += res;
    /* now check for and end-of-message marker: three nuls */
    for(j=2;j<c->read_buf_cursor;j++)
    {
        if (c->read_buf[j-2]==0 && c->read_buf[j-1]==0 && c->read_buf[j]==0)
        {
            c->read_buf_request=j-1;
            end_found=1;
            break;
        }
    }
    if (end_found && (c->read_buf_request > 0) )
    {
        request_process(c, cs);

        if (c->keep_alive)
        {
            /* shift the buffer so that the unprocessed part is at 0 */
            for(j=c->read_buf_request; j<c->read_buf_cursor; j++)
            {
                c->read_buf[j - c->read_buf_request] = c->read_buf[j];
            }
            c->read_buf_cursor -= c->read_buf_request;
            c->read_buf_request=0;
        }
    }

    return res;
}


