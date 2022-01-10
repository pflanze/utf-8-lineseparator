OPT ?= -O2
SAN ?= -fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer
CFLAGS ?= -Wall -gdwarf-4 -g3 $(OPT) -std=c11 -fdiagnostics-color=always
compile = $(CC) -DAFL=0 $(CFLAGS)
AFL_CLANG_FAST ?= afl-clang-fast

# For *cov* targets, using
# https://clang.llvm.org/docs/SourceBasedCodeCoverage.html :
CLANG ?= clang
COVFLAGS ?= -O0 -fprofile-instr-generate -fcoverage-mapping


headers = bufferedstream.h buffer.h env.h io.h leakcheck.h maybe.h mem.h monkey.h result.h shorttypenames.h string.h string_perror.h unicode.h util.h
binaries = utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.afl utf-8-lineseparator.aflsan utf-8-lineseparator.cov utf-8-lineseparator.aflcov test test.san


utf-8-lineseparator: utf-8-lineseparator.c $(headers)
	$(compile) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c $(headers)
	$(compile) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

utf-8-lineseparator.afl: utf-8-lineseparator.c $(headers)
	$(AFL_CLANG_FAST) $(CFLAGS) -DAFL=1 -o utf-8-lineseparator.afl utf-8-lineseparator.c

utf-8-lineseparator.aflsan: utf-8-lineseparator.c $(headers)
	$(AFL_CLANG_FAST) $(CFLAGS) $(SAN) -DAFL=1 -o utf-8-lineseparator.aflsan utf-8-lineseparator.c

utf-8-lineseparator.cov: utf-8-lineseparator.c $(headers)
	$(CLANG) $(CFLAGS) $(COVFLAGS) -DAFL=0 -o utf-8-lineseparator.cov utf-8-lineseparator.c

utf-8-lineseparator.aflcov: utf-8-lineseparator.c $(headers)
	$(AFL_CLANG_FAST) $(CFLAGS) $(COVFLAGS) -DAFL=1 -o utf-8-lineseparator.aflcov utf-8-lineseparator.c

test: test.c $(headers)
	$(compile) -o test test.c

test.san: test.c $(headers)
	$(compile) $(SAN) -o test.san test.c


all: $(binaries)


runaflnosan: utf-8-lineseparator.afl
	BIN=./utf-8-lineseparator.afl bin/run-afl

runafl: utf-8-lineseparator.aflsan
	BIN=./utf-8-lineseparator.aflsan bin/run-afl

runaflcov: utf-8-lineseparator.cov
	BIN=./utf-8-lineseparator.cov bin/runaflcov


runtests: test.san
	./test.san

runtestsgdb: test
	gdbrun ./test


clean:
	rm -f $(binaries) *.profdata
	rm -rf ./*.profraw/

.PHONY: clean
