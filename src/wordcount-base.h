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

#ifndef IS_WORDCOUNT_BASE_H
#define IS_WORDCOUNT_BASE_H

#include <lib-common/core.h>
#include <lib-common/container-qvector.h>

#include "wordcount.iop.h"

/** Unpack the server configuration from IOP YAML file.
 *
 * \param[in]  server_cfg_path The path to the server configuration in IOP
 *                             YAML.
 * \param[out] err             The error description in case of error.
 * \return The unpacked IOP configuration allocated on the t_scope, or NULL in
 *         case of error.
 */
wordcount__server_cfg__t * nullable
t_wordcount_unpack_server_cfg(const char *server_cfg_path, sb_t *err);

/** Module to handle common behaviour between server and client.
 *
 * Handle an event loop blocker and handle termination signals.
 * Depends on ic module.
 */
MODULE_DECLARE(wordcount_base);

#endif /* IS_WORDCOUNT_BASE_H */
