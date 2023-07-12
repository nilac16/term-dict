#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L
#   warning "_POSIX_C_SOURCE will be defined as 200809"
#   define _POSIX_C_SOURCE 200809L
#endif

#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 500
#   warning "_XOPEN_SOURCE will be defined as 500"
#   define _XOPEN_SOURCE 500
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ftw.h>
#include <libgen.h>
#include <sys/time.h>

#include "cache.h"
#include "log.h"

/** The maximum number of chars used for stack buffers containing paths  */
#define PATHLEN 260

/** The column size of each word printed from the cache with -l, --list. Make
 *  sure that this is a divisor of 80, and it should be larger than 3 to
 *  facilitate ellipsizing
 */
#define LISTLEN 16

/** The maximum number of allowed entries in the disk cache. Each word appears
 *  to be about 1 kB
 */
#define CACHE_MAX 200


static struct {
    char dir[PATHLEN];

    int    count;
    time_t lru;
} cache = { 0 };


/** @brief Retrieves the maximum number of files allowed in the cache */
static int cache_max(void)
{
    return CACHE_MAX;
}


/** @brief Checks if the cache directory was set */
static int cache_ready(void)
{
    return cache.dir[0] != '\0';
}


/** @brief Helper function checking for stdlib errors and truncation */
static int cache_snprintf(char *dst, size_t len, const char *restrict fmt, ...)
{
    va_list args;
    int res;

    va_start(args, fmt);
    res = vsnprintf(dst, len, fmt, args);
    va_end(args);
    if (res < 0) {
        dict_perror("Failed creating cache path");
    } else if ((unsigned)res >= len) {
        dict_logs(DICT_ERROR, "Cache path truncated");
    } else {
        res = 0;
    }
    return res;
}


int cache_init(void)
{
    const char *home;
    int res = 1;

    home = getenv("HOME");
    if (home) {
        res = cache_snprintf(cache.dir, sizeof cache.dir, "%s/.local/share/dict/cache", home);
        if (res) {
            memset(cache.dir, 0, sizeof cache.dir);
        } else {
            
        }
    } else {
        dict_logs(DICT_WARN, "No HOME dir found, cannot use cache");
    }
    return res;
}


/** Updates the file's last-accessed time to right now */
static int cache_touch(FILE *fp)
{
    struct timespec ts[2];
    struct stat sbuf;
    int fd, res = 1;

    fd = fileno(fp);
    if (fd != -1) {
        res = fstat(fd, &sbuf)
           || clock_gettime(CLOCK_REALTIME, &ts[0]);
        if (!res) {
            ts[1] = sbuf.st_mtim;
            res = futimens(fd, ts);
        }
        if (res) {
            dict_perror("Cannot update cache time");
        }
    } else {
        dict_perror("Cannot get cache descriptor");
    }
    return res;
}


/** Opens the file at @p path and reads as much of its data as possible into
 *  @p buf
 */
static int cache_open_read(char *buf, size_t *len, const char *path)
{
    int res = 0;
    FILE *fp;

    fp = fopen(path, "rb");
    if (fp) {
        *len = fread(buf, 1UL, *len, fp);
        if (ferror(fp)) {
            dict_perror("Cannot read cache");
            *len = 0;
            res = 1;
        } else {
            cache_touch(fp);
        }
        fclose(fp);
    } else {
        res = errno != ENOENT;  /* no file no problem */
        *len = 0;
        if (res) {
            dict_perror("Failed to open cache entry");
        }
    }
    return res;
}


int cache_lookup(const char *word, char *buf, size_t *len)
{
    char path[PATHLEN];
    int res = 0;

    if (!cache_ready()) {
        return 0;
    }
    if (cache_snprintf(path, sizeof path, "%s/%s", cache.dir, word)) {
        return 1;
    }
    res = cache_open_read(buf, len, path);
    return res;
}


static time_t mintime(time_t x, time_t y)
{
    return (x < y) ? x : y;
}


/** @brief FTW callback that counts the number of cache files and saves the
 *      earliest timestamp for later eviction
 */
static int cache_ftw_count(const char        *path,
                           const struct stat *sbuf,
                           int                type)
{
    (void)path;

    if (type == FTW_F) {
        cache.count++;
        cache.lru = mintime(cache.lru, sbuf->st_atime);
    }
    return 0;
}


/** @brief FTW callback that removes the file with the previously written
 *      earliest timestamp
 */
static int cache_ftw_remove(const char        *path,
                            const struct stat *sbuf,
                            int                type)
{
    if (type == FTW_F) {
        if (sbuf->st_atime == cache.lru) {
            remove(path);
            return 1;
        }
    }
    return 0;
}


/** @brief Walks the cache directory and removes the earliest file, if and only
 *      if the cache is full
 */
static int cache_evict(void)
{
    cache.count = 0;
    cache.lru = LONG_MAX;
    ftw(cache.dir, cache_ftw_count, 1);
    if (cache.count > cache_max()) {
        ftw(cache.dir, cache_ftw_remove, 1);
    } else {
        
    }
    return 0;
}


/** @brief Writes the reply to the opened cache file @p fp */
static int cache_flush(const char *reply, const char *path, FILE *fp)
{
    int res;

    res = fputs(reply, fp) == EOF;
    fclose(fp);
    if (res) {
        dict_perror("Failed to flush reply to disk");
        if (remove(path)) {
            dict_perror("Cannot delete empty cache file");
        }
    }
    return res;
}


int cache_write(const char *word, const char *reply)
{
    char path[PATHLEN];
    int res = 1;
    FILE *fp;

    if (!cache_ready()) {
        return 0;
    }
    if (cache_snprintf(path, sizeof path, "%s/%s", cache.dir, word)) {
        return 1;
    }
    fp = fopen(path, "wb");
    if (fp) {
        cache_evict();
        res = cache_flush(reply, path, fp);
    } else {
        dict_perror("Cannot open cache file for writing");
    }
    return res;
}


static struct {
    FILE *fp;

    size_t   size;
    unsigned count;
} listctx = { 0 };


/** @brief Writes @p name to @p buf, writing no more than @p buflen chars.
 *      @p name is copied with a leading space, and the remaining space in the
 *      buffer is filled with whitespace. If @p name would be truncated, it will
 *      instead be ellipsized
 *  @returns Negative on error, zero if @p name was copied entirely, and
 *      positive if @p name was truncated and ellipsized
 */
static int cache_ellipsize(char *buf, size_t buflen, const char *name)
{
    int res;

    assert(buflen > 3); /* You dork I told you so many ways how to avoid this */

    res = snprintf(buf, buflen, " %s", name);
    if (res < 0) {
        dict_perror(NULL);
    } else if ((unsigned)res >= buflen) {
        strcpy(buf + buflen - 4, "...");
    } else {
        memset(buf + (unsigned)res, ' ', buflen - res);
        buf[buflen - 1] = '\0';
        res = 0;
    }
    return res;
}


/** @brief FTW callback that computes directory size on disk and prints each
 *      filename
 */
static int cache_ftw_list(const char        *path,
                          const struct stat *sbuf,
                          int                type)
{
    const unsigned listlen = 80 / LISTLEN;
    char buf[PATHLEN], name[LISTLEN + 1];

    (void)sbuf;

    static_assert(LISTLEN >= 3);      /* Cannot accomodate unsafe ellipsizing */
    static_assert(80 % LISTLEN == 0); /* Word length not divisible by 80 */

    if (cache_snprintf(buf, sizeof buf, "%s", path)) {
        return 1;
    }
    if (type == FTW_F) {
        if (listctx.count && !(listctx.count % listlen)) {
            fputc('\n', listctx.fp);
        }
        cache_ellipsize(name, sizeof name, basename(buf));
        fputs(name, listctx.fp);
        listctx.size += sbuf->st_size;
        listctx.count++;
    }
    return 0;
}


/** @brief Formats bytes in engineering notation */
static void format_bytes(size_t *bytes, const char **prefix)
{
    static const char *si[] = {
        "", "k", "M", "G"
    };
    const size_t len = sizeof si / sizeof *si;
    unsigned i = 0;

    while (*bytes > 1000 && i < len) {
        *bytes = (*bytes + 500) / 1000;
        i++;
    }
    *prefix = si[i];
}


void cache_list(FILE *fp)
{
    const char *si;

    if (!cache_ready()) {
        dict_logs(DICT_ERROR, "Cannot list cache dir: Not initialized");
        return;
    }
    /* fputs("The disk cache contains definitions for the following words:", fp); */
    listctx.fp = fp;
    ftw(cache.dir, cache_ftw_list, 1);
    format_bytes(&listctx.size, &si);
    fprintf(fp, "\n\nThe cache contains %u words, and is using %zu %sB of disk space. Use -f, --force\nto refresh a cached entry.\n", listctx.count, listctx.size, si);
    memset(&listctx, 0, sizeof listctx);
}


int cache_remove(const char *word)
{
    char path[PATHLEN];

    if (!cache_ready()) {
        dict_logf(DICT_ERROR, "Cannot delete %s: Cache was not initialized", word);
        return -1;
    }
    if (cache_snprintf(path, sizeof path, "%s/%s", cache.dir, word)) {
        return -1;
    }
    errno = 0;
    if (remove(path) && errno != ENOENT) {
        dict_perror("Failed to delete cache entry");
        return -1;
    }
    return errno == ENOENT;
}
