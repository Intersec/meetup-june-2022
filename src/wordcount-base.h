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


/* Structure to contain the occurrences for a unique word */
typedef struct wordcount_word_occurrences_t {
    lstr_t word;
    unsigned occurrences;
} wordcount_word_occurrences_t;

/* Create the vector type to store the word occurrences. */
qvector_t(word_occurrences_vec, wordcount_word_occurrences_t);


#endif /* IS_WORDCOUNT_BASE_H */
