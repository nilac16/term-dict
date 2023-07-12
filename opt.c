#include <stdio.h>
#include <string.h>

#include "opt.h"
#include "log.h"


enum {
    OPT_NONE,
    OPT_SHORT,
    OPT_LONG
};

static int arg_type(const char *arg)
{
    if (arg[0] == '-') {
        switch (arg[1]) {
        case '-':
            return (arg[2]) ? OPT_LONG : OPT_NONE;
        case '\0':
            return OPT_NONE;
        default:
            return OPT_SHORT;
        }
    }
    return OPT_NONE;
}


static int dict_opt_word(const char *word, struct options *opt)
{
    if (opt->word) {
        return 1;
    } else {
        opt->word = word;
        return 0;
    }
}


static int dict_opt_short(const char *optstr, struct options *opt)
{
    static const char *shorts = "fhlrs";
    const char *i;
    int res = 0;
    char c;

    for (; *optstr; optstr++) {
        i = strchr(shorts, *optstr);
        c = (i) ? *i : '\0';
        switch (c) {
        case 'f':
            opt->force = true;
            break;
        case 'h':
            opt->help = true;
            break;
        case 'l':
            opt->list_history = true;
            break;
        case 'r':
            opt->remove = true;
            break;
        case 's':
            opt->skip = true;
            break;
        default:
            dict_logf(DICT_WARN, "Unrecognized short option %c", *optstr);
            res = 1;
        }
    }
    return res;
}


static int dict_opt_long(const char *longopt, struct options *opt)
{
    static const char *longs[] = {
        "force",
        "help",
        "list",
        "remove",
        "skip"
    };
    int res = 0;

    if (!strcmp(longopt, longs[0])) {
        opt->force = true;
    } else if (!strcmp(longopt, longs[1])) {
        opt->help = true;
    } else if (!strcmp(longopt, longs[2])) {
        opt->list_history = true;
    } else if (!strcmp(longopt, longs[3])) {
        opt->remove = true;
    } else if (!strcmp(longopt, longs[4])) {
        opt->skip = true;
    } else {
        dict_logf(DICT_WARN, "Unrecognized long option %s", longopt);
        res = 1;
    }
    return res;
}


void dict_opt_parse(int argc, char *argv[], struct options *opt)
{
    int idx = 0;

    while (++idx < argc) {
        switch (arg_type(argv[idx])) {
        case OPT_NONE:
            dict_opt_word(argv[idx], opt);
            break;
        case OPT_SHORT:
            dict_opt_short(argv[idx] + 1, opt);
            break;
        case OPT_LONG:
            dict_opt_long(argv[idx] + 2, opt);
            break;
        }
    }
}


const char *dict_opt_string(void)
{
    static const char *opts =
    "  -f, --force      always make a web request, do not use the cache\n"
    "  -h, --help       show this help message\n"
    "  -l, --list       list the entries currently in the cache\n"
    "  -r, --remove     remove WORD from the cache\n"
    "  -s, --skip       do not save this definition to the disk cache\n";

    return opts;
}
