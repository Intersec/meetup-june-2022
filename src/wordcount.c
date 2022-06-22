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


static const char *short_args_g = "<file_path>";

static const char *long_usage_g[] = {
    "Count the number of occurrences of each unique words in the file",
    "located at <file_path>.",
    NULL,
};

static struct {
    bool opt_help;
} wordcount_g;
#define _G wordcount_g

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

/** Split the file content per word.
 *
 * For now, we just print each word in the file.
 *
 * \param[in] file_content The file content.
 */
static void wordcount_split_words(lstr_t file_content)
{
    pstream_t file_ps;

    /* Initialize the stream parser from the content */
    file_ps = ps_initlstr(&file_content);

    /* Iterate until the stream parser is empty */
    while (!ps_done(&file_ps)) {
        pstream_t word_ps;

        /* Get the next word */
        word_ps = ps_get_span(&file_ps, &ctype_iswordpart);

        /* Skip the characters to the next word for next loop */
        ps_skip_cspan(&file_ps, &ctype_iswordpart);

        /* Do nothing if word is empty */
        if (ps_done(&word_ps)) {
            continue;
        }

        /* Just print the word for now */
        e_info("%*pM", PS_FMT_ARG(&word_ps));
    }
}

int main(int argc, char **argv)
{
    const char *arg0 = NEXTARG(argc, argv);
    const char *file_path;
    lstr_t file_content;

    /* Parse the arguments */
    argc = parseopt(argc, argv, opts_g, 0);
    if (argc != 1 || _G.opt_help) {
        makeusage(_G.opt_help ? 0 : -1, arg0, short_args_g,
                  long_usage_g, opts_g);
    }

    /* Get the file content */
    file_path = NEXTARG(argc, argv);
    RETHROW(wordcount_get_file_content(file_path, &file_content));

    /* Split the file content per word */
    wordcount_split_words(file_content);

    /* Clean-up */
    lstr_wipe(&file_content);

    return 0;
}
