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


static const char *short_args_g = "-c <server_cfg_path>";

static const char *long_usage_g[] = {
    "Server part of wordcount",
    "",
    "Receive the file content from the client via RPC, count the number of ",
    "occurrences of each unique words, and send the results back",
    "",
    "The configuration of the server is expected to be in IOP YAML as ",
    "described by the IOP `wordcount.ServerCfg`",
    NULL,
};

static struct {
    bool opt_help;
    const char *opt_cfg_path;

    /* IChannel server */
    el_t ic_server;

    /* RPC implementations table */
    qm_t(ic_cbs) ic_impl;
} wordcount_server_g;
#define _G wordcount_server_g

static popt_t opts_g[] = {
    OPT_GROUP("Options:"),
    OPT_FLAG('h', "help", &_G.opt_help, "show this help"),
    OPT_STR('c', "cfg", &_G.opt_cfg_path,
            "path to the server configuration in YAML"),
    OPT_END()
};

/* Create the map type lstr_t => unsigned with name word_occurrences.
 * The word equality ignore the case. */
qm_kvec_t(word_occurrences_map, lstr_t, unsigned, qhash_lstr_ascii_ihash,
          qhash_lstr_ascii_iequal);

/* Create the vector type to store the word occurrences. */
qvector_t(word_occurrences_vec, wordcount__word_occurrences__t);

/** Split the file content per word and count their occurrences.
 *
 * Put the words and their occurrences in a map.
 *
 * \param[in]  file_content         The file content.
 * \param[out] word_occurrences_map The map countaining the words and their
 *                                  occurrences.
 */
static void wordcount_split_words(
    lstr_t file_content, qm_t(word_occurrences_map) *word_occurrences_map)
{
    pstream_t file_ps;

    /* Initialize the stream parser from the content */
    file_ps = ps_initlstr(&file_content);

    /* Iterate until the stream parser is empty */
    while (!ps_done(&file_ps)) {
        pstream_t word_ps;
        lstr_t word_lstr;
        uint32_t pos;

        /* Get the next word */
        word_ps = ps_get_span(&file_ps, &ctype_iswordpart);

        /* Skip the characters to the next word for next loop */
        ps_skip_cspan(&file_ps, &ctype_iswordpart);

        /* Do nothing if word is empty */
        if (ps_done(&word_ps)) {
            continue;
        }

        /* Put the word in the map */
        /* Store 1 as value if `word_lstr` is not already in the map... */
        word_lstr = LSTR_PS_V(&word_ps);
        pos = qm_put(word_occurrences_map, word_occurrences_map, &word_lstr,
                     1, 0);
        if (pos & QHASH_COLLISION) {
            /* ...increment the value by 1 if `word_lstr` is already in the
             * map. */
            word_occurrences_map->values[pos ^ QHASH_COLLISION] += 1;
        }
    }
}

/** Sort the words by their occurrences in the map to a vector.
 *
 * The words are converted to lower case.
 *
 * \param[in]  word_occurrences_map The map countaining the words and their
 *                                  occurrences.
 * \param[out] word_occurrences_vec The vector of sorted words by their
 *                                  occurrences.
 *                                  The vector and words are allocated on the
 *                                  t_scope.
 */
static void t_wordcount_sort_word_occurrences(
    const qm_t(word_occurrences_map) *word_occurrences_map,
    qv_t(word_occurrences_vec) *word_occurrences_vec)
{
    /* Initialize the vector on the t_scope to the size of the map in order to
     * do only one allocation */
    t_qv_init(word_occurrences_vec,
              qm_len(word_occurrences_map, word_occurrences_map));

    /* Populate the vector with the map content */
    qm_for_each_key_value(word_occurrences_map, word, occurrences,
                          word_occurrences_map)
    {
        wordcount__word_occurrences__t word_occurrences = {
            /* Duplicate the word on the t_scope and use lower case */
            .word = t_lstr_ascii_tolower(word),
            .occurrences = occurrences,
        };

        qv_append(word_occurrences_vec, word_occurrences);
    }

    /* Sort the vector by occurrences.
     * Use iop_sort() which does some introspection to get the sorting fields.
     */
    iop_sort(wordcount__word_occurrences, word_occurrences_vec->tab,
             word_occurrences_vec->len, LSTR("occurrences"), IOP_SORT_REVERSE,
             NULL);
}

/** Split the words from the content of a file and sort the words by
 * occurrences.
 *
 * \param[in]  file_content         The file content.
 * \param[out] word_occurrences_vec The vector of sorted words by their
 *                                  occurrences.
 *                                  The vector is allocated on the t_scope.
 */
static void t_wordcount_split_and_sort_word_occurrences(
    lstr_t file_content, qv_t(word_occurrences_vec) *word_occurrences_vec)
{
    qm_t(word_occurrences_map) word_occurrences_map;

    /* Initialize the map of word occurrences */
    qm_init(word_occurrences_map, &word_occurrences_map);

    /* Split the file content per word */
    wordcount_split_words(file_content, &word_occurrences_map);

    /* Sort the words by their occurrences */
    t_wordcount_sort_word_occurrences(&word_occurrences_map,
                                      word_occurrences_vec);

    /* Clean-up */
    qm_wipe(word_occurrences_map, &word_occurrences_map);
}

/** RPC implementation, this function is called on RPC query. */
static void IOP_RPC_IMPL(wordcount__mod, wordcount_iface, count_occurrences)
{
    t_scope;
    qv_t(word_occurrences_vec) word_occurrences_vec;

    /* Split the words from the content of a file and sort the words by
     * occurrences */
    t_wordcount_split_and_sort_word_occurrences(
        arg->file_content, &word_occurrences_vec);

    /* Send the word occurrences back.
     * The vector is converted as an IOP array. */
    ic_reply(ic, slot, wordcount__mod, wordcount_iface, count_occurrences,
             .word_occurrences = IOP_TYPED_ARRAY_TAB(
                 wordcount__word_occurrences, &word_occurrences_vec));
}

/** Called on client status changes. */
static void wordcount_server_on_event(ichannel_t *ic, ic_event_t evt)
{
    if (evt == IC_EVT_CONNECTED) {
        e_notice("client %p connected", ic);
    } else
    if (evt == IC_EVT_DISCONNECTED) {
        e_warning("client %p disconnected", ic);
    }
}

/** Called on incoming client connection. */
static int wordcount_server_on_accept(el_t ev, int fd)
{
    ichannel_t *ic;

    e_info("incoming connection");
    ic              = ic_new();
    ic->on_event    = &wordcount_server_on_event;
    ic->impl        = &_G.ic_impl;
    ic->do_el_unref = true;

    ic_spawn(ic, fd, NULL);
    return 0;
}

/** Initialization callback called when the module is required.
 *
 * \param[in] arg  An optional argument provided to the module.
 *                 In our case, since no argument is passed to the module, it
 *                 is NULL.
 * \return -1 in case of error, 0 in case of success.
 */
static int wordcount_server_initialize(void *nullable arg)
{
    t_scope;
    SB_1k(err);
    wordcount__server_cfg__t *server_cfg;
    sockunion_t su;

    e_info("starting server");

    /* Unpack the server configuration */
    server_cfg = t_wordcount_unpack_server_cfg(_G.opt_cfg_path, &err);
    if (!server_cfg) {
        e_error("unable to unpack the server cfg `%s`: %pL",
                _G.opt_cfg_path, &err);
        return -1;
    }

    /* Initialize the RPC implementations table */
    qm_init(ic_cbs, &_G.ic_impl);

    /* Get the socket union from the address */
    if (addr_info_str(&su, server_cfg->address.s, server_cfg->port,
                      AF_UNSPEC) < 0)
    {
        e_error("unable to resolve address %pL:%d", &server_cfg->address,
                server_cfg->port);
        return -1;
    }

    /* Start the IChannel server */
    _G.ic_server = ic_listento(&su, SOCK_STREAM, IPPROTO_TCP,
                               &wordcount_server_on_accept);
    if (!_G.ic_server) {
        e_error("cannot bind on %pL:%d", &server_cfg->address,
                server_cfg->port);
        return -1;
    }

    /* Register the RPC */
    ic_register(&_G.ic_impl, wordcount__mod, wordcount_iface,
                count_occurrences);

    return 0;
}

/** Called on termination signals.
 *
 * \param[in] signo The signal number
 */
static void wordcount_server_on_term(int signo)
{
    /* Release the server on termination signal. */
    el_unregister(&_G.ic_server);
}

/** Shutdown callback called when the module is released.
 *
 * \return -1 in case of error, 0 in case of success.
 */
static int wordcount_server_shutdown(void)
{
    e_info("stopping server");

    /* Clean-up the RPC implementations table */
    qm_wipe(ic_cbs, &_G.ic_impl);
    return 0;
}

/** The definition of the module */
static MODULE_BEGIN(wordcount_server)
    /* wordcount_base module is initialized before this module, and released
     * after this module */
    MODULE_DEPENDS_ON(wordcount_base);

    /* Implement module method to react on termination signals */
    MODULE_IMPLEMENTS_INT(on_term, wordcount_server_on_term);
MODULE_END()

int main(int argc, char **argv)
{
    const char *arg0 = NEXTARG(argc, argv);

    /* Parse the arguments */
    argc = parseopt(argc, argv, opts_g, 0);
    if (argc != 0 || _G.opt_help || !_G.opt_cfg_path) {
        makeusage(_G.opt_help ? 0 : -1, arg0, short_args_g,
                  long_usage_g, opts_g);
    }

    /* Initialize the server */
    MODULE_REQUIRE(wordcount_server);

    /* Start the event loop and block until the event loop blocker of
     * wordcount_base module is unregistered */
    el_loop();

    /* Shutdown the server */
    MODULE_RELEASE(wordcount_server);

    return 0;
}
