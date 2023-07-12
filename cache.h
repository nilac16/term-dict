#pragma once

#ifndef DICT_CACHE_H
#define DICT_CACHE_H

#include <stddef.h>


/** @brief Initializes any resources required by the caching system
 *  @returns Nonzero on error
 */
int cache_init(void);


/** @brief Searches the word cache for @p word. If found writes the cached reply
 *      to @p buf
 *  @param word
 *      Word the search for
 *  @param[out] buf
 *      Buffer to write the reply to, if it exists
 *  @param[in,out] len
 *      On input, the maximum size of @p buf. On ouput, the number of bytes
 *      written to @p buf. If this is zero, the word was not found
 *  @returns Nonzero on error, and zero on success. Zero will be returned even
 *      if the word is not cached; you must use the resulting value of @p len
 *      to distinguish a successful cache hit from a miss
 *  @note A cache hit will update the file's last-accessed time, influencing its
 *      eviction order
 */
int cache_lookup(const char *word, char *buf, size_t *len);


/** @brief Writes @p word and its associated @p reply to the cache
 *  @param word
 *      Word
 *  @param reply
 *      Verbatim reply from dictionaryapi.dev
 *  @returns Nonzero on error. This function does not report which entry was
 *      evicted, if any
 */
int cache_write(const char *word, const char *reply);


/** @brief Walks the cache directory and lists each word inside in order
 *  @param fp
 *      FILE * to output to
 */
void cache_list(FILE *fp);


/** @brief Removes @p word from the cache, if it exists
 *  @param word
 *      Word to be removed
 *  @returns Negative on error, zero on success, and positive if @p word was not
 *      found
 */
int cache_remove(const char *word);


#endif /* DICT_CACHE_H */
