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


#ifndef __SRV_CONFIG_H__
#define __SRV_CONFIG_H__

#include <sys/stat.h>

/* the client + server settings. These are read from the basedir that's
    specified on the command line of the client, and passed on to the server
    once it is spawend. */
typedef struct suq_config
{
    int ntask;              /** max number of processors to run on */
    unsigned int next_id;   /** next job id */

    char *socket_filename;  /** socket file name
                                default: /tmp/suq.<uid>/<hostname>.socket
                                Note that we don't use XDG_RUNTIME_DIR as it's
                                likely to be deleted when the user logs out.*/

    char *config_filename;  /** config file name
                                default: $XDG_CONFIG_HOME/suq/<hostname>.conf
                                where XDG_CONFIG_HOME=$HOME/.config if not
                                set */

    char *log_dir;          /** log dir name.
                                default: $XDG_STATE_HOME/suq
                                where $XDG_STATE_HOME=$HOME/.local/state if not
                                set */

    char *output_dir;       /** job output (stdout & stderr) dir.
                                default: /tmp/suq.<uid> */

    char *hostname;         /** hostname as determined at server startup. */

    int dirty;              /** whether the config needs to be written back
                                before the server is shut down. */
} suq_config;

/** Make all directories up until the last slash, if they don't exist.
  * 'raises' fatal_system_error() if there is an error creating any of the
  * directories.
  *
  * @param path     The path to make. Will make all directories up to the last
  *                 path separator ('/').
  * @param mode     The permissions mode.
  */
void makedirs(const char *path, mode_t mode);


/** Get the standard file name
  *
  * @param given_filename   An optional file name that is given as a client
  *                         argument that overrides the default filename.
  * @returns    A malloced string with the configuration file name.
  */
char *suq_config_get_filename(const char *given_filename);


/** initialize suq_config, and read from filename.
  *
  * @param sc       The pre-allocated config object to initialize.
  * @param filename The file name to read config from.*/
void suq_config_init(suq_config *sc, const char *filename);

/** deallocate config object contents.
  *
  * @param sc   The config object whose objects to deallocate.
  */
void suq_config_destroy(suq_config *sc);

/** write suq config to file. The file name is given in the config
  * object itself and can be changed with suq_config_set_config_filename().
  *
  * @param sc   The config object to write. */
void suq_config_write(suq_config *sc);

/** Make the directories for the socket, the log dir and the output dir.
  * This must be called at server creation to ensure the existence of the
  * directories.
  *
  * @param sc       The config object to get the dir names from.
  */
void suq_config_makedirs(const suq_config *sc);

/** Get the server log filename, based on the log_dir.
  *
  * @param sc       The config object to use.
  * @returns        A malloced string with the server log name.
  */
char *suq_config_get_server_log_name(const suq_config *sc);

/** Get the job log filename, based on the log_dir.
  *
  * @param sc       The config object to use.
  * @returns        A malloced string with the job log name.
  */
char *suq_config_get_job_log_name(const suq_config *sc);

/** Set the max. number of tasks for the server.
  *
  * @param sc       The config object to change.
  * @param ntask    The maximum number of concurrent tasks to run.
  */
void suq_config_set_ntask(suq_config *sc, int ntask);

/** get the next job ID
  *
  * @param sc       The config object
  * @returns        A unique job ID
  */
int suq_config_get_next_id(suq_config *sc);

#endif
