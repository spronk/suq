## Process this file with automake to produce Makefile.in

bin_PROGRAMS = suq

suq_SOURCES = client_conn.c     main.c          server.c \
              connection.c      srv_config.c  	log_err.c \
              request.c         sig_handler.c   job.c \
              request_process.c wait.c 		usage.c

AM_CPPFLAGS =  -DDATADIR=\"$(datadir)\"
