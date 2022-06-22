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


int main(int argc, char **argv)
{
    const char *arg0 = NEXTARG(argc, argv);
    const char *file_path;

    /* Parse the arguments */
    argc = parseopt(argc, argv, opts_g, 0);
    if (argc != 1 || _G.opt_help) {
        makeusage(_G.opt_help ? 0 : -1, arg0, short_args_g,
                  long_usage_g, opts_g);
    }

    file_path = NEXTARG(argc, argv);
    e_info("TODO wordcount on file `%s`", file_path);

    return 0;
}
