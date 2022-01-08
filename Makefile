OPT ?= -O2
SAN ?= -fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer
CFLAGS ?= -Wall -gdwarf-4 -g3 $(OPT) -std=c11 -fdiagnostics-color=always
compile = $(CC) -DAFL=0 $(CFLAGS)

headers = bufferedstream.h buffer.h env.h io.h leakcheck.h maybe.h mem.h result.h shorttypenames.h string.h string_perror.h unicode.h util.h
binaries = utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.afl utf-8-lineseparator.aflsan test test.san

utf-8-lineseparator: utf-8-lineseparator.c $(headers)
	$(compile) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c $(headers)
	$(compile) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

utf-8-lineseparator.afl: utf-8-lineseparator.c $(headers)
	afl-clang-fast $(CFLAGS) -DAFL=1 -o utf-8-lineseparator.afl utf-8-lineseparator.c

utf-8-lineseparator.aflsan: utf-8-lineseparator.c $(headers)
	afl-clang-fast $(CFLAGS) $(SAN) -DAFL=1 -o utf-8-lineseparator.aflsan utf-8-lineseparator.c

test: test.c $(headers)
	$(compile) -o test test.c

test.san: test.c $(headers)
	$(compile) $(SAN) -o test.san test.c


all: $(binaries)


runaflnosan: utf-8-lineseparator.afl
	BIN=./utf-8-lineseparator.afl bin/run-afl

runafl: utf-8-lineseparator.aflsan
	bin/run-afl

runtestsgdb: test
	gdbrun ./test

runtests: test.san
	./test.san


clean:
	rm -f $(binaries)

.PHONY: clean
