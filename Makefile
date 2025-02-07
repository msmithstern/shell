CC = gcc
CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align 
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror -D_GNU_SOURCE

PROMPT = -DPROMPT

EXECS = 33sh 33noprompt
FILE = sh.c jobs.c
HEADER = sh.h jobs.h

.PHONY: clean all

all: $(EXECS)
		
33sh: $(FILE)
	$(CC) $(CFLAGS) -DPROMPT $^ -o $@

33noprompt: $(FILE)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(EXECS)

