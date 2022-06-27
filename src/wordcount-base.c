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

#include <lib-common/el.h>
#include <lib-common/iop-rpc.h>
#include <lib-common/iop-yaml.h>

#include "wordcount-base.h"

static struct {
    /* Event loop blocker */
    el_t blocker;
} wordcount_base_g;
#define _G wordcount_base_g

wordcount__server_cfg__t * nullable
t_wordcount_unpack_server_cfg(const char *server_cfg_path, sb_t *err)
{
    wordcount__server_cfg__t *res = NULL;

    RETHROW_NP(t_iop_yunpack_ptr_file(server_cfg_path,
                                      &wordcount__server_cfg__s,
                                      (void **)&res, 0, NULL, err));
    return res;
}

/** Callback called when a termination signal is received.
 *
 * \param[in] el     The event loop signal handler.
 * \param[in] signum The signal number.
 * \param[in] priv   The data passed on signal register.
 */
static void wordcount_base_on_term(el_t el, int signum, data_t priv)
{
    /* Unregister blocker to exit the event loop */
    el_unregister(&_G.blocker);
}

/** Initialization callback called when the module is required.
 *
 * \param[in] arg  An optional argument provided to the module.
 *                 In our case, since no argument is passed to the module, it
 *                 is NULL.
 * \return -1 in case of error, 0 in case of success.
 */
static int wordcount_base_initialize(void *nullable arg)
{
    /* Register the termination signals */
    el_signal_register(SIGTERM, &wordcount_base_on_term, NULL);
    el_signal_register(SIGINT,  &wordcount_base_on_term, NULL);
    el_signal_register(SIGQUIT, &wordcount_base_on_term, NULL);

    /* Register event loop blocker */
    _G.blocker = el_blocker_register();
    return 0;
}

/** Shutdown callback called when the module is released.
 *
 * \return -1 in case of error, 0 in case of success.
 */
static int wordcount_base_shutdown(void)
{
    return 0;
}

/** The definition of the module */
MODULE_BEGIN(wordcount_base)
    MODULE_DEPENDS_ON(ic);
MODULE_END()
