#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <json-c/json.h>

#include "json.h"
#include "color.h"

#define DICT_WORD        "word"
#define DICT_PHONETIC    "phonetic"
#define DICT_PHONETICS   "phonetics"
#define DICT_PHONTEXT    "text"
#define DICT_MEANINGS    "meanings"
#define DICT_CATEGORY    "partOfSpeech"
#define DICT_DEFINITIONS "definitions"
#define DICT_DEFINITION  "definition"
#define DICT_SYNONYMS    "synonyms"
#define DICT_ANTONYMS    "antonyms"

/** No line shall overrun this maximum length (except the part of speech, whose
 *  length remains inauspiciously unchecked)
 */
#define JSON_MAXCOLUMNS 80

#define INDENT_PRTOFSPCH    " "
#define INDENT_ANT_SYN      "      "


static int json_maxcolumns(void)
{
    return JSON_MAXCOLUMNS;
}


/** @brief Trim prefixed whitespace from @p nptr
 *  @returns A pointer to the first non-whitespace char after @p nptr
 */
static const char *json_ltrim(const char *nptr)
{
    while (isspace(*nptr)) {
        nptr++;
    }
    return nptr;
}


/** @brief Trim prefixed whitespace from @p nptr, then finds the first
 *      whitespace/nul character thereafter
 *  @returns A pointer to the first character of the next word, or the nul term
 */
static const char *json_wordboundary(const char *nptr)
{
    nptr = json_ltrim(nptr);
    while (*nptr && !isspace(*nptr)) {
        nptr++;
    }
    return nptr;
}


/** @brief Print words until the next word would overflow @p width
 *  @param nptr
 *      String to be printed
 *  @param width
 *      Maximum number of columns allowed to print before a line break is
 *      required
 *  @returns The first nonwhitespace char not printed
 */
static const char *json_print_clipped(const char *nptr, int width)
{
    const char *end, *next;

    next = json_wordboundary(nptr);
    do {
        end = next;
        next = json_wordboundary(next);
    } while (*end && next - nptr <= width);
    fwrite(nptr, 1, end - nptr, stdout);
    putchar('\n');
    return json_ltrim(end);
}


/** @brief Print itemized definition without exceeding column width determined
 *      at compile time
 *  @param def
 *      Definition string. The first line of this definition will be prefixed
 *      with a single hyphen '-'. Indented six characters
 */
static void json_print_def(const char *def)
{
    static const char bullet[] = "  - ", indent[] = "    ";
    int clipwidth;

    clipwidth = json_maxcolumns() - (sizeof bullet - 1);
    fputs(bullet, stdout);
    def = json_print_clipped(def, clipwidth);
    while (*def) {
        fputs(indent, stdout);
        def = json_print_clipped(def, clipwidth);
    }
}


/** @brief Prints a word to stdout as part of a comma-delimited list, without
 *      overrunning the column limit
 *  @param word
 *      Word to be written
 *  @param remword
 *      The words remaining after @p word
 *  @param indent
 *      The indentation to place on a new line, should this function need to
 *      create one
 *  @param[in,out] space
 *      The remaining chars available for use on this line, on both input and
 *      output
 */
static void json_commalist_print_word(const char *word,
                                      size_t      remword,
                                      int         indent,
                                      int        *space)
{
    bool comma = remword != 0;
    int len;

    /* The comma must be attached to the word */
    len = (int)strlen(word) + (int)comma;
    if (len > *space) {
        printf("\n%*s", indent, "");
        *space = json_maxcolumns() - indent;
    }
    *space -= len;
    //assert(*space >= 0);    /* Indent is too large */
    fputs(word, stdout);
    if (comma) {
        putchar(',');
        if (*space > 0) {
            *space -= 1;
            putchar(' ');
        }
    }
}


typedef const char *json_textfn_t(struct json_object *);


/** @brief Prints a comma-delimited list from the JSON array @p arr. This
 *      function always line-breaks. Output is line-broken at word boundaries
 *      before overrunning the classic 80-column limit. The indentation is
 *      computed from the size of @p head
 *  @param arr
 *      Array node
 *  @param colr
 *      Color code to be used for this list. The @p head is always printed in
 *      bold; successive elements of the list are printed using just the color
 *  @param head
 *      A string that precedes the comma delimitation. Unless @p always is set,
 *      this string is not written out if @p arr is empty. If the array is not
 *      empty, this will be proceeded by a color and a space. The indentation
 *      computed from this string includes the following colon and space chars
 *  @param textfn
 *      A function pointer that takes the array node and, by some great mystery
 *      (to this function) produces a pointer to text to be written to stdout
 *      for this node. If this returns NULL, that array node is skipped
 *  @param always
 *      If this is true, then always write the header, even if @p arr is empty
 *  @returns The number of entries from @p arr printed
 */
static int json_print_commalist(struct json_object *arr,
                                unsigned            colr,
                                const char         *head,
                                json_textfn_t      *textfn,
                                bool                always)
{
    int indent, space, res = 0;
    const char *text;
    size_t N, i = 0;

    N = json_object_array_length(arr);
    if (N) {
        color_send(stdout, colr | COLOR_BOLD);
        printf("%s: %n", head, &indent);
        color_reset(stdout);
        color_send(stdout, colr);
        space = json_maxcolumns() - indent;
        do {
            N--;
            text = textfn(json_object_array_get_idx(arr, i++));
            if (text) {
                json_commalist_print_word(text, N, indent, &space);
                res++;
            }
        } while (N);
        color_reset(stdout);
        putchar('\n');
    } else if (always) {
        color_send(stdout, colr | COLOR_BOLD);
        puts(head);
        color_reset(stdout);
    }
    return res;
}


static int json_print_synonyms(struct json_object *syn, int indent)
{
    char buf[81];
    const unsigned color = COLOR_GREEN | COLOR_INTENSE;
    /* const char *header = INDENT_ANT_SYN "Synonyms"; */

    sprintf(buf, "%*sSynonyms", indent, "");
    return json_print_commalist(syn, color, buf, json_object_get_string, false);
}


static int json_print_antonyms(struct json_object *ant, int indent)
{
    char buf[81];
    const unsigned color = COLOR_RED | COLOR_INTENSE;
    /* const char *header = INDENT_ANT_SYN "Antonyms"; */

    sprintf(buf, "%*sAntonyms", indent, "");
    return json_print_commalist(ant, color, buf, json_object_get_string, false);
}


/** @brief Print all definitions under this category in itemized format
 *  @returns The number of synonyms and antonyms found under "definitions"
 */
static int json_print_definitions(struct json_object *defs)
{
    struct json_object *def, *val;
    int nsynant = 0;
    size_t N, i;

    N = json_object_array_length(defs);
    for (i = 0; i < N; i++) {
        def = json_object_array_get_idx(defs, i);
        json_object_object_get_ex(def, DICT_DEFINITION, &val);
        json_print_def(json_object_get_string(val));
        json_object_object_get_ex(def, DICT_SYNONYMS, &val);
        nsynant += json_print_synonyms(val, 6);
        json_object_object_get_ex(def, DICT_ANTONYMS, &val);
        nsynant += json_print_antonyms(val, 6);
    }
    return nsynant;
}


static void json_print_partofspeech(struct json_object *val)
{
    const char *text;

    text = json_object_get_string(val);
    fprintf(stdout, INDENT_PRTOFSPCH "[" ANSI_BOLD ANSI_YELLOWHI "%s" ANSI_RESET "]\n", text);
}


/** @brief Print all parts of speech */
static void json_print_meanings(struct json_object *meanings)
{
    struct json_object *meaning, *val;
    size_t N, i;

    N = json_object_array_length(meanings);
    for (i = 0; i < N; i++) {
        meaning = json_object_array_get_idx(meanings, i);
        json_object_object_get_ex(meaning, DICT_CATEGORY, &val);
        json_print_partofspeech(val);
        json_object_object_get_ex(meaning, DICT_DEFINITIONS, &val);
        if (!json_print_definitions(val)) {
            json_object_object_get_ex(meaning, DICT_SYNONYMS, &val);
            json_print_synonyms(val, 4);
            json_object_object_get_ex(meaning, DICT_ANTONYMS, &val);
            json_print_antonyms(val, 4);
        }
        putchar('\n');
    }
}


/** @brief Callback function for the commalist routine */
static const char *json_get_phonetic(struct json_object *phon)
{
    const char *res = NULL;

    if (json_object_object_get_ex(phon, DICT_PHONTEXT, &phon)) {
        res = json_object_get_string(phon);
    }
    return res;
}


/** @brief Deletes all nodes that do not contain a "text" node */
static void json_remove_textless(struct json_object *arr)
{
    struct json_object *node, *text;
    size_t N, i;

    N = json_object_array_length(arr);
    for (i = 0; i < N; i++) {
        node = json_object_array_get_idx(arr, i);
        json_object_object_get_ex(node, DICT_PHONTEXT, &text);
        if (!text) {
            json_object_array_del_idx(arr, i, 1);
            i--;
            N--;
        }
    }
}


/** @brief Prints the word and its associated list of pronunciations to stdout.
 *      This function always line breaks
 */
static void json_print_word(struct json_object *word)
{
    const unsigned colr = COLOR_CYAN | COLOR_INTENSE;
    struct json_object *phon;
    const char *name;

    json_object_object_get_ex(word, DICT_PHONETICS, &phon);
    json_remove_textless(phon);
    if (json_object_object_get_ex(word, DICT_WORD, &word)) {
        name = json_object_get_string(word);
        json_print_commalist(phon, colr, name, json_get_phonetic, true);
    }
    putchar('\n');
}


static int json_print_definition(struct json_object *json)
{
    struct json_object *word, *val;
    size_t N, i;

    if (json_object_get_type(json) != json_type_array) {
        return 1;
    }
    N = json_object_array_length(json);
    for (i = 0; i < N; i++) {
        word = json_object_array_get_idx(json, i);
        json_print_word(word);
        json_object_object_get_ex(word, DICT_MEANINGS, &val);
        json_print_meanings(val);
    }
    return 0;
}


int dict_print_JSON(const char *jsonstr, const char *jsonend)
{
    struct json_object *json;
    struct json_tokener *tok;
    int res;

    tok = json_tokener_new();
    json = json_tokener_parse_ex(tok, jsonstr, jsonend - jsonstr);
    json_tokener_free(tok);
    res = json_print_definition(json);
    json_object_put(json);
    return res;
}
