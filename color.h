#pragma once

#ifndef DICT_COLOR_H
#define DICT_COLOR_H

#include <stdio.h>

#define ANSI_RESET  "\e[0m"

#define ANSI_BLACK  "\e[30m"
#define ANSI_RED    "\e[31m"
#define ANSI_GREEN  "\e[32m"
#define ANSI_YELLOW "\e[33m"
#define ANSI_BLUE   "\e[34m"
#define ANSI_PURPLE "\e[35m"
#define ANSI_CYAN   "\e[36m"
#define ANSI_WHITE  "\e[37m"

#define ANSI_BLACKHI  "\e[90m"
#define ANSI_REDHI    "\e[91m"
#define ANSI_GREENHI  "\e[92m"
#define ANSI_YELLOWHI "\e[93m"
#define ANSI_BLUEHI   "\e[94m"
#define ANSI_PURPLEHI "\e[95m"
#define ANSI_CYANHI   "\e[96m"
#define ANSI_WHITEHI  "\e[97m"

#define ANSI_BOLD   "\e[1m"
#define ANSI_UNBOLD "\e[22m"


#define COLOR_INTENSE 0x80
#define COLOR_BOLD    0x40

enum {
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_WHITE
};


/** @brief Send a virtual terminal color code to @p fp
 *  @param fp
 *      FILE to send to
 *  @param code
 *      COLOR_* Bitflags specifying the color
 */
void color_send(FILE *fp, unsigned code);


/** @brief Reset the terminal color */
static inline void color_reset(FILE *fp) { fputs(ANSI_RESET, fp); }


#endif /* DICT_COLOR_H */
