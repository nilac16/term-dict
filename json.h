#pragma once

#ifndef DICT_JSON_H
#define DICT_JSON_H


/** @brief Reads a JSON between @p begin and @p end and prints relevant semantic
 *      information contained therein to stdout
 *  @returns Nonzero if no definition is available
 */
int dict_print_JSON(const char *begin, const char *end);


#endif /* DICT_JSON_H */
