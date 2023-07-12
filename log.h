#pragma once

#ifndef DICT_LOG_H
#define DICT_LOG_H

#include <stdio.h>


typedef enum dict_loglvl {
    DICT_DEBUG,
    DICT_INFO,
    DICT_WARN,
    DICT_ERROR
} loglvl_t;


/** @brief Issues a message to stdout/stderr */
int dict_logs(loglvl_t lvl, const char *msg);


/** @brief Issues formatted output to stdout/stderr */
int dict_logf(loglvl_t lvl, const char *fmt, ...);


/** @brief Issues a perror message to stderr, using appropriate formatting.
 *      Use NULL to just print the strerror
 */
void dict_perror(const char *msg);


#endif /* DICT_LOG_H */
