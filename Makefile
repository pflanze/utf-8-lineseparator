OPT ?= -O2
SAN ?= -fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer
CFLAGS ?= -Wall -gdwarf-4 -g3 $(OPT) -std=c11 -fdiagnostics-color=always
compile = $(CC) -DAFL=0 $(CFLAGS)

headers = buffer.h env.h io.h maybe.h mem.h result.h shorttypenames.h string.h util.h

utf-8-lineseparator: utf-8-lineseparator.c $(headers)
	$(compile) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c $(headers)
	$(compile) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

utf-8-lineseparator.afl: utf-8-lineseparator.c $(headers)
	afl-clang-fast $(CFLAGS) $(SAN) -DAFL=1 -o utf-8-lineseparator.afl utf-8-lineseparator.c

afl: utf-8-lineseparator.afl

all: utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.afl


runafl: afl
	bin/run-afl


clean:
	rm -f utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.afl

.PHONY: clean
