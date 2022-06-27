/***************************************************************************/
/*                                                                         */
/* Copyright 2022 INTERSEC SA                                              */
/*                                                                         */
/* Licensed under the Apache License, Version 2.0 (the "License");         */
/* you may not use this file except in compliance with the License.        */
/* You may obtain a copy of the License at                                 */
/*                                                                         */
/*     http://www.apache.org/licenses/LICENSE-2.0                          */
/*                                                                         */
/* Unless required by applicable law or agreed to in writing, software     */
/* distributed under the License is distributed on an "AS IS" BASIS,       */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/* See the License for the specific language governing permissions and     */
/* limitations under the License.                                          */
/*                                                                         */
/***************************************************************************/

#include <lib-common/core.h>
#include <lib-common/parseopt.h>
#include <lib-common/iop-rpc.h>

#include "wordcount-base.h"

static const char *short_args_g = "-c <server_cfg_path> <file_path>";

static const char *long_usage_g[] = {
    "Client part of wordcount",
    "",
    "Read the content of the file located at <file_path> and send it to the ",
    "server to get the number of occurrences of each unique words via RPC.",
    "",
    "The configuration of the server is expected to be in IOP YAML as ",
    "described by the IOP `wordcount.ServerCfg`",
    NULL,
};

static struct {
    bool opt_help;
    const char *opt_cfg_path;

    /** The exit status status of the main function */
    int exit_res;

    /** The path of the file where to get the content from */
    const char *file_path;

    /** Remote ichannel */
    ichannel_t  remote_ic;
} wordcount_client_g;
#define _G wordcount_client_g

static popt_t opts_g[] = {
    OPT_GROUP("Options:"),
    OPT_FLAG('h', "help", &_G.opt_help, "show this help"),
    OPT_STR('c', "cfg", &_G.opt_cfg_path,
            "path to the server configuration in YAML"),
    OPT_END()
};

/** Get file content of file located at \p file_path.
 *
 * The file is mmapped, and thus the content of the file is not duplicated in
 * memory.
 * Thanks to that, parsing big files is not an issue.
 * However, this can still be an issue if the content of the file is passed
 * as is in an RPC.
 *
 * The file content must be wiped after use to munmap the file.
 *
 * \param[in]  file_path    The path to the file to read.
 * \param[out] file_content The content of the file. It must be wiped after
 *                          use.
 * \return -1 in case of error, 0 otherwise.
 */
static int wordcount_get_file_content(const char *file_path,
                                      lstr_t *file_content)
{
    if (lstr_init_from_file(file_content, file_path, PROT_READ,
                            MAP_PRIVATE) < 0)
    {
        e_error("unable to get the content of the file `%s` has failed: %m\n",
                file_path);
        return -1;
    }

    return 0;
}

/** Exit the client with the given result status code.
 *
 * \param[in] res The result status code to be used as exit status for the
 *                main() fucntion.
 */
static void worcount_client_exit(int res)
{
    if (el_is_terminating()) {
        return;
    }

    _G.exit_res = res;

    /* Send termination signal to ourself */
    kill(0, SIGQUIT);
}

/** Called when the RPC query is finished with the RPC result if the RPC is
 * successful. */
static void IOP_RPC_CB(wordcount__mod, wordcount_iface, count_occurrences)
{
    if (status != IC_MSG_OK) {
        /* RPC error */
        const char *error = ic_status_to_string(status);

        e_error("RPC error: %s", error);
        worcount_client_exit(-1);
        return;
    }

    /* Display the sorted word occurrences */
    tab_for_each_ptr(word_occurrences, &res->word_occurrences) {
        e_info("%pL => %u", &word_occurrences->word,
               word_occurrences->occurrences);
    }

    /* Exit the client */
    worcount_client_exit(0);
}

/** Called when the client is connected to the server. */
static void wordcount_client_on_connect(void)
{
    lstr_t file_content;

    /* Get the file content */
    if (wordcount_get_file_content(_G.file_path, &file_content) < 0) {
        worcount_client_exit(-1);
        return;
    }

    /* Send the file content to the server via RPC */
    ic_query2(&_G.remote_ic, ic_msg_new(0), wordcount__mod, wordcount_iface,
              count_occurrences, .file_content = file_content);

    /* Clean-up */
    lstr_wipe(&file_content);
}

/** Called on server status changes. */
static void wordcount_client_on_event(ichannel_t *ic, ic_event_t evt)
{
    if (evt == IC_EVT_CONNECTED) {
        e_notice("connected to server");
        wordcount_client_on_connect();
    } else if (evt == IC_EVT_DISCONNECTED && !el_is_terminating()) {
        e_warning("disconnected from server");
        worcount_client_exit(-1);
    }
}

/** Initialization callback called when the module is required.
 *
 * \param[in] arg  An optional argument provided to the module.
 *                 In our case, since no argument is passed to the module, it
 *                 is NULL.
 * \return -1 in case of error, 0 in case of success.
 */
static int wordcount_client_initialize(void *nullable arg)
{
    t_scope;
    SB_1k(err);
    wordcount__server_cfg__t *server_cfg;

    e_info("starting client");

    /* Unpack the server configuration */
    server_cfg = t_wordcount_unpack_server_cfg(_G.opt_cfg_path, &err);
    if (!server_cfg) {
        e_error("unable to unpack the server cfg `%s`: %pL",
                _G.opt_cfg_path, &err);
        return -1;
    }

    /* Create the remote ichannel to connect to the server */
    ic_init(&_G.remote_ic);
    _G.remote_ic.on_event = &wordcount_client_on_event;

    /* Get the socket union from the address */
    if (addr_info_str(&_G.remote_ic.su, server_cfg->address.s,
                      server_cfg->port, AF_UNSPEC) < 0)
    {
        e_error("unable to resolve address %pL:%d", &server_cfg->address,
                server_cfg->port);
        return -1;
    }

    /* Connect to the server */
    if (ic_connect(&_G.remote_ic) < 0) {
        e_error("cannot connect to %pL:%d", &server_cfg->address,
                server_cfg->port);
        return -1;
    }

    return 0;
}

/** Called on termination signals.
 *
 * \param[in] signo The signal number
 */
static void wordcount_client_on_term(int signo)
{
    ic_bye(&_G.remote_ic);
}

/** Shutdown callback called when the module is released.
 *
 * \return -1 in case of error, 0 in case of success.
 */
static int wordcount_client_shutdown(void)
{
    e_info("stopping client");
    ic_wipe(&_G.remote_ic);
    return 0;
}

/** The definition of the module */
static MODULE_BEGIN(wordcount_client)
    /* wordcount_base module is initialized before this module, and released
     * after this module */
    MODULE_DEPENDS_ON(wordcount_base);

    /* Implement module method to react on termination signals */
    MODULE_IMPLEMENTS_INT(on_term, wordcount_client_on_term);
MODULE_END()

int main(int argc, char **argv)
{
    const char *arg0 = NEXTARG(argc, argv);

    /* Parse the arguments */
    argc = parseopt(argc, argv, opts_g, 0);
    if (argc != 1 || _G.opt_help || !_G.opt_cfg_path) {
        makeusage(_G.opt_help ? 0 : -1, arg0, short_args_g,
                  long_usage_g, opts_g);
    }

    _G.file_path = NEXTARG(argc, argv);

    /* Initialize wordcount_client module */
    MODULE_REQUIRE(wordcount_client);

    /* Start the event loop and block until the event loop blocker of
     * wordcount_base module is unregistered */
    el_loop();

    /* Shutdown the client */
    MODULE_RELEASE(wordcount_client);

    return _G.exit_res;
}
