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

CFLAGS ?= -Wall -gdwarf-4 -g3 $(OPT) $(STD) -fdiagnostics-color=always
compile = $(CC) -DAFL=0 $(CFLAGS)
AFL_CLANG_FAST ?= afl-clang-fast

# For *cov* targets, using
# https://clang.llvm.org/docs/SourceBasedCodeCoverage.html :
CLANG ?= clang
COVFLAGS ?= -O0 -fprofile-instr-generate -fcoverage-mapping


headers = Array.h BufferedStream.h Buffer.h env.h io.h leakcheck.h Maybe.h mem.h monkey.h monkey-posix.h Result.h shorttypenames.h Slice.h String.h String_perror.h test_BufferedStream.h testinfra.h test_unicode.h unicode.h util.h
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


afl/1:
	mkdir -p afl
	echo > afl/1

runaflnosan: utf-8-lineseparator.afl afl/1
	BIN=./utf-8-lineseparator.afl bin/run-afl

runafl: utf-8-lineseparator.aflsan afl/1
	BIN=./utf-8-lineseparator.aflsan bin/run-afl

runaflcov: utf-8-lineseparator.cov
	bin/runaflcov ./utf-8-lineseparator.cov aflfind


runtests: test.san
	./test.san

runtestsgdb: test
	gdbrun ./test

check: runtests

checkgdb: runtestsgdb


%.E: %.c
	$(compile) -E -o $@ $<
%.E.c: %.E
	perl -wne 'next if /^# +\d+/; print or die $!' < $< | clang-format > $@
expand: utf-8-lineseparator.E.c test.E.c


clean:
	rm -f $(binaries) *.profdata
	rm -rf ./*.profraw/

.PHONY: clean
