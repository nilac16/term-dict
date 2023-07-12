CFLAGS  := -O2 -Wall -Wextra
LIBS    := -ljson-c -lcurl
DEFINES := -D_POSIX_C_SOURCE=200809 -D_XOPEN_SOURCE=500

dict: dict.c json.c opt.c cache.c color.c log.c
	$(CC) -o dict $^ $(CFLAGS) $(LIBS) $(DEFINES)

release: dict.c json.c opt.c cache.c color.c log.c
	$(CC) -o dict $^ $(CFLAGS) $(LIBS) $(DEFINES) -DNDEBUG
