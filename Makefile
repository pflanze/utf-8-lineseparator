OPT ?= -O2
SAN ?= -fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer

ifeq ($(CC),g++)
  STD ?= -std=c++17
else
  ifeq ($(CC),clang++)
    STD ?= -std=c++17
  else
    STD ?= -std=c11
  endif
endif

CFLAGS ?= -Wall -gdwarf-4 -g3 $(OPT) -fdiagnostics-color=always
compile = $(CC) $(STD) -DAFL=0 $(CFLAGS)
AFL_CLANG_FAST ?= afl-clang-fast
compileafl = $(AFL_CLANG_FAST) -DAFL=1 $(CFLAGS)

# For *cov* targets, using
# https://clang.llvm.org/docs/SourceBasedCodeCoverage.html :
CLANG ?= clang
COVFLAGS ?= -O0 -fprofile-instr-generate -fcoverage-mapping


headers = Vec.h BufferedStream.h Buffer.h env.h io.h leakcheck.h LSlice.h macro-util.h mem.h monkey.h monkey-posix.h Option.h Result.h shorttypenames.h Slice.h String.h String_perror.h test_BufferedStream.h testinfra.h test_unicode.h unicode.h util.h
binaries = utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.afl utf-8-lineseparator.aflsan utf-8-lineseparator.cov utf-8-lineseparator.aflcov test test.san


utf-8-lineseparator: utf-8-lineseparator.c $(headers)
	$(compile) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c $(headers)
	$(compile) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

utf-8-lineseparator.afl: utf-8-lineseparator.c $(headers)
	$(compileafl) -o utf-8-lineseparator.afl utf-8-lineseparator.c

utf-8-lineseparator.aflsan: utf-8-lineseparator.c $(headers)
	$(compileafl) $(SAN) -o utf-8-lineseparator.aflsan utf-8-lineseparator.c

utf-8-lineseparator.cov: utf-8-lineseparator.c $(headers)
	$(CLANG) $(CFLAGS) $(COVFLAGS) -DAFL=0 -o utf-8-lineseparator.cov utf-8-lineseparator.c

utf-8-lineseparator.aflcov: utf-8-lineseparator.c $(headers)
	$(compileafl) $(COVFLAGS) -o utf-8-lineseparator.aflcov utf-8-lineseparator.c

test: test.c $(headers)
	$(compile) -o test test.c

test.san: test.c $(headers)
	$(compile) $(SAN) -o test.san test.c


all: $(binaries)


afl/1:
	mkdir -p afl
	echo > afl/1

runaflnosan: utf-8-lineseparator.afl afl/1
	BIN=./utf-8-lineseparator.afl bin/run-afl

runafl: utf-8-lineseparator.aflsan afl/1
	BIN=./utf-8-lineseparator.aflsan bin/run-afl

runaflcov: utf-8-lineseparator.cov
	bin/runaflcov ./utf-8-lineseparator.cov aflfind


runtests: test.san utf-8-lineseparator
	./runtests

runtestsgdb: test
	gdbrun ./test

check: runtests

checkgdb: runtestsgdb

checkall:
	make clean
	CC=gcc make all check
	make clean
	CC=g++ make all check
	make clean
	CC=clang make all check
	make clean
	make all check

%.E: %.c
	$(compile) -E -o $@ $<
%.E.c: %.E
	perl -wne 'next if /^# +\d+/; print or die $!' < $< | clang-format > $@
expand: utf-8-lineseparator.E.c test.E.c


clean:
	rm -f $(binaries) *.profdata utf-8-lineseparator.E.c test.E.c
	rm -rf ./*.profraw/

.PHONY: clean runtests checkall

