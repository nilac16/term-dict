#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>

#include "opt.h"
#include "json.h"
#include "cache.h"
#include "log.h"


static size_t dict_write_cb(char *ptr, size_t size, size_t nmemb, void *usrdata)
{
    char **dptr = usrdata;

    (void)size;

    memcpy(*dptr, ptr, nmemb);
    *dptr += nmemb;
    return nmemb;
}


static char downloadbuf[65536];


static int dict_get_def(CURL *hcurl, struct options *opt)
{
    char *dptr = downloadbuf;
    char url[128];
    CURLcode result;

    snprintf(url, sizeof url, "https://api.dictionaryapi.dev/api/v2/entries/en/%s", opt->word);
    curl_easy_setopt(hcurl, CURLOPT_URL, url);
    curl_easy_setopt(hcurl, CURLOPT_WRITEFUNCTION, dict_write_cb);
    curl_easy_setopt(hcurl, CURLOPT_WRITEDATA, &dptr);
    result = curl_easy_perform(hcurl);
    if (!result) {
        *dptr = '\0';
        if (dict_print_JSON(downloadbuf, dptr)) {
            dict_logf(DICT_ERROR, "Could not look up word \"%s\"", opt->word);
            dict_logs(DICT_ERROR, "No lexical information available");
        } else if (!opt->skip && cache_write(opt->word, downloadbuf)) {
            dict_logf(DICT_ERROR, "Failed to write %s to cache", opt->word);
        }
    } else {
        dict_logf(DICT_ERROR, "curl: 0x%04x: %s", result, curl_easy_strerror(result));
    }
    return result;
}


static int dict_prep_curl(struct options *opt)
{
    int res = 1;
    CURL *hcurl;

    hcurl = curl_easy_init();
    if (hcurl) {
        res = dict_get_def(hcurl, opt);
        curl_easy_cleanup(hcurl);
    } else {
        dict_logs(DICT_ERROR, "Could not initialize curl");
    }
    return res;
}


static void dict_print_usage(void)
{
    static const char *usage =
    "Usage: dict [OPTION] [WORD]\n"
    "Fetch the dictionary entry for WORD from dictionaryapi.dev\n\n"
    "Options:\n";

    fputs(usage, stdout);
    fputs(dict_opt_string(), stdout);
    fputc('\n', stdout);
}


static void dict_list(struct options *opt)
{
    (void)opt;  /* I may wish to inspect other options later */
    cache_init();
    cache_list(stdout);
}


/** @brief Attempts to fetch a definition from the cache, using the web API as a
 *      fallback in case of cache miss or error
 */
static void dict_try_cache(struct options *opt)
{
    size_t len = sizeof downloadbuf;
    bool hit = false;

    if (!cache_lookup(opt->word, downloadbuf, &len) && len) {
        dict_print_JSON(downloadbuf, downloadbuf + len);
        hit = true;
    } else {
        dict_prep_curl(opt);
    }
    if (hit) {
        puts("(cached reply; use -f, --force to refresh)");
    }
}


static void dict_lookup(struct options *opt)
{
    cache_init();
    if (opt->remove) {
        if (cache_remove(opt->word) > 0) {
            dict_logf(DICT_ERROR, "Word %s not found in cache", opt->word);
        }
    } else if (opt->force) {
        dict_prep_curl(opt);
    } else {
        dict_try_cache(opt);
    }
}


int main(int argc, char *argv[])
{
    struct options opt = { 0 };
    int res = 0;

    dict_opt_parse(argc, argv, &opt);
    if (0) {
        /* I know this looks dumb but I'm doing it to facilitate moving things
        around */

    } else if (opt.help) {
        dict_print_usage();

    } else if (opt.list_history) {
        dict_list(&opt);

    } else if (opt.word) {
        dict_lookup(&opt);

    } else {

        dict_logs(DICT_ERROR, "Invalid call to dict: Word required");
        dict_print_usage();
        res = 1;
    }
    return res;
}
