## Process this file with automake to produce Makefile.in

bin_PROGRAMS = suq

suq_SOURCES = client_conn.c     main.c          server.c \
              connection.c      settings.c  	err.c \
              request.c         sig_handler.c   job.c \
              request_process.c wait.c 		usage.c

AM_CPPFLAGS =  -DDATADIR=\"$(datadir)\"
