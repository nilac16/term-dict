#pragma once

#ifndef DICT_OPTIONS_H
#define DICT_OPTIONS_H

#include <stdbool.h>


struct options {
    const char *word;

    /** These are listed in order of precedence */
    bool list_history;  /* Walk the cache dir and print each word */
    bool remove;        /* Delete WORD from the cache */
    bool force;         /* Always call the REST API, do not use the cache */
    bool skip;          /* Do not cache this definition */
    bool help;          /* Show usage */
};


/** @brief Parse cmd args into the options context struct @p opt */
void dict_opt_parse(int argc, char *argv[], struct options *opt);


/** @brief Returns a pointer to a string giving the options list, in a format
 *      typically seen in a shell command usage string.
 *  @details This only returns the indented list of options and effects, in
 *      particular, it is missing the "Options:" header commonly seen in its
 *      progenitors
 */
const char *dict_opt_string(void);


#endif /* DICT_OPTIONS_H */
