#include "color.h"


void color_send(FILE *fp, unsigned code)
{
    static const unsigned mask = 0xF;
    static const char *seqlo[] = {
        ANSI_BLACK, ANSI_RED,    ANSI_GREEN, ANSI_YELLOW,
        ANSI_BLUE,  ANSI_PURPLE, ANSI_CYAN,  ANSI_WHITE };
    static const char *seqhi[] = {
        ANSI_BLACKHI, ANSI_REDHI,    ANSI_GREENHI, ANSI_YELLOWHI,
        ANSI_BLUEHI,  ANSI_PURPLEHI, ANSI_CYANHI,  ANSI_WHITEHI };
    static const char **seq[] = { seqlo, seqhi };
    unsigned intensity = (code & COLOR_INTENSE) != 0;
    unsigned offset = code & mask;

    if (code & COLOR_BOLD) {
        fputs(ANSI_BOLD, fp);
    }
    fputs(seq[intensity][offset], fp);
}
