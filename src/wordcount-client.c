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

#include "wordcount-base.h"

static const char *short_args_g = "<file_path>";

static const char *long_usage_g[] = {
    "Client part of wordcount",
    "",
    "Read the content of the file located at <file_path> and send it to the ",
    "server to get the number of occurrences of each unique words via RPC.",
    NULL,
};

static struct {
    bool opt_help;
    const char *file_path;
} wordcount_client_g;
#define _G wordcount_client_g

static popt_t opts_g[] = {
    OPT_GROUP("Options:"),
    OPT_FLAG('h', "help", &_G.opt_help, "show this help"),
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

__unused__
static void wordcount_client_on_rpc_cb(
    const qv_t(word_occurrences_vec) *word_occurrences_vec)
{
    /* Display the sorted word occurrences */
    tab_for_each_ptr(word_occurrences, word_occurrences_vec) {
        e_info("%pL => %u", &word_occurrences->word,
               word_occurrences->occurrences);
    }
}

__unused__
static void wordcount_client_on_connect(void)
{
    lstr_t file_content;

    /* Get the file content */
    if (wordcount_get_file_content(_G.file_path, &file_content) < 0) {
        /* TODO: exit */
        return;
    }

    /* TODO: send the file content to the server and wait for the result */

    /* Clean-up */
    lstr_wipe(&file_content);
}

int main(int argc, char **argv)
{
    const char *arg0 = NEXTARG(argc, argv);

    /* Parse the arguments */
    argc = parseopt(argc, argv, opts_g, 0);
    if (argc != 1 || _G.opt_help) {
        makeusage(_G.opt_help ? 0 : -1, arg0, short_args_g,
                  long_usage_g, opts_g);
    }

    _G.file_path = NEXTARG(argc, argv);

    /* TODO: connect to the server */

    return 0;
}
