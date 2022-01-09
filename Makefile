OPT ?= -O2
SAN ?= -fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer
CFLAGS ?= -Wall -gdwarf-4 -g3 $(OPT) -std=c11 -fdiagnostics-color=always
compile = $(CC) -DAFL=0 $(CFLAGS)
COVFLAGS ?= --coverage
ifeq ($(CC), clang)
    covflags_gcc=$(COVFLAGS)
else
    covflags_gcc=$(COVFLAGS) -fkeep-inline-functions -fkeep-static-functions
endif

headers = bufferedstream.h buffer.h env.h io.h leakcheck.h maybe.h mem.h result.h shorttypenames.h string.h string_perror.h unicode.h util.h
binaries = utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.afl utf-8-lineseparator.aflsan utf-8-lineseparator.cov utf-8-lineseparator.aflcov test test.san

utf-8-lineseparator: utf-8-lineseparator.c $(headers)
	$(compile) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c $(headers)
	$(compile) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

utf-8-lineseparator.afl: utf-8-lineseparator.c $(headers)
	afl-clang-fast $(CFLAGS) -DAFL=1 -o utf-8-lineseparator.afl utf-8-lineseparator.c

utf-8-lineseparator.aflsan: utf-8-lineseparator.c $(headers)
	afl-clang-fast $(CFLAGS) $(SAN) -DAFL=1 -o utf-8-lineseparator.aflsan utf-8-lineseparator.c

utf-8-lineseparator.cov: utf-8-lineseparator.c $(headers)
	$(compile) -O0 $(COVFLAGS) -o utf-8-lineseparator.cov utf-8-lineseparator.c
	rm -f utf-8-lineseparator.gcno

utf-8-lineseparator.aflcov: utf-8-lineseparator.c $(headers)
	afl-clang-fast $(CFLAGS) -O0 $(COVFLAGS) -DAFL=1 -o utf-8-lineseparator.aflcov utf-8-lineseparator.c
	rm -f utf-8-lineseparator.gcno

test: test.c $(headers)
	$(compile) -o test test.c

test.san: test.c $(headers)
	$(compile) $(SAN) -o test.san test.c


all: $(binaries)


runaflnosan: utf-8-lineseparator.afl
	BIN=./utf-8-lineseparator.afl bin/run-afl

runafl: utf-8-lineseparator.aflsan
	bin/run-afl

runaflcov: utf-8-lineseparator.aflcov
	BIN=./utf-8-lineseparator.aflcov bin/run-afl

runtestsgdb: test
	gdbrun ./test

runtests: test.san
	./test.san


clean:
	rm -f $(binaries)

.PHONY: clean
