#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "log.h"
#include "color.h"


int dict_logs(loglvl_t lvl, const char *msg)
{
    static const char *colors[] = {
        ANSI_BLUEHI,
        "",
        ANSI_YELLOWHI,
        ANSI_REDHI
    };
    static const char *prefix[] = {
        ANSI_GREENHI ANSI_BOLD "debug: ",
        "",
        ANSI_YELLOWHI ANSI_BOLD "warning: ",
        ANSI_REDHI ANSI_BOLD "error: "
    };
    FILE *fp = (lvl < DICT_WARN) ? stdout : stderr;
    unsigned i = (unsigned)lvl;
    int res;
    
    res = fprintf(fp, ANSI_BOLD "dict: " ANSI_RESET "%s" ANSI_RESET "%s%s\n",
                                                    prefix[i], colors[i], msg);
    color_reset(fp);
    return res;
}


int dict_logf(loglvl_t lvl, const char *fmt, ...)
{
    char buf[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    return dict_logs(lvl, buf);
}


void dict_perror(const char *msg)
{
    char ebuf[256];

    strerror_r(errno, ebuf, sizeof ebuf);
    if (msg) {
        dict_logf(DICT_ERROR, "%s: %s", msg, ebuf);
    } else {
        dict_logs(DICT_ERROR, ebuf);
    }
}
