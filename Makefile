SAN=-fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer
CFLAGS=-Wall -gdwarf-4 -g3 -O2 -std=c11 -fdiagnostics-color=always
compile=$(CC) -DFUZZ=0 $(CFLAGS)

utf-8-lineseparator: utf-8-lineseparator.c
	$(compile) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c
	$(compile) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

utf-8-lineseparator.fuzz: utf-8-lineseparator.c
	afl-clang-fast $(CFLAGS) $(SAN) -DFUZZ=1 -o utf-8-lineseparator.fuzz utf-8-lineseparator.c

fuzz: utf-8-lineseparator.fuzz
	bin/run-afl


clean:
	rm -f utf-8-lineseparator utf-8-lineseparator.san utf-8-lineseparator.fuzz

.PHONY: clean
