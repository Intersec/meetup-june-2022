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

/* Create the map type lstr_t => unsigned with name word_occurrences */
qm_kvec_t(word_occurrences_map, lstr_t, unsigned, qhash_lstr_hash,
          qhash_lstr_equal);

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

int main(int argc, char **argv)
{
    const char *arg0 = NEXTARG(argc, argv);
    const char *file_path;
    lstr_t file_content;
    qm_t(word_occurrences_map) word_occurrences_map;

    /* Parse the arguments */
    argc = parseopt(argc, argv, opts_g, 0);
    if (argc != 1 || _G.opt_help) {
        makeusage(_G.opt_help ? 0 : -1, arg0, short_args_g,
                  long_usage_g, opts_g);
    }

    /* Get the file content */
    file_path = NEXTARG(argc, argv);
    RETHROW(wordcount_get_file_content(file_path, &file_content));

    /* Initialize the map of word occurrences */
    qm_init(word_occurrences_map, &word_occurrences_map);

    /* Split the file content per word */
    wordcount_split_words(file_content, &word_occurrences_map);

    /* Print the map content */
    qm_for_each_key_value(word_occurrences_map, word, occurrences,
                          &word_occurrences_map)
    {
        e_info("%pL => %u", &word, occurrences);
    }

    /* Clean-up */
    qm_wipe(word_occurrences_map, &word_occurrences_map);
    lstr_wipe(&file_content);

    return 0;
}
