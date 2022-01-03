SAN=-fsanitize=undefined -fsanitize=address -fPIE -fno-omit-frame-pointer

CC=gcc -Wall -gdwarf-4 -g3 -O2 -std=c11 -fdiagnostics-color=always

utf-8-lineseparator: utf-8-lineseparator.c
	$(CC) -o utf-8-lineseparator utf-8-lineseparator.c

utf-8-lineseparator.san: utf-8-lineseparator.c
	$(CC) $(SAN) -o utf-8-lineseparator.san utf-8-lineseparator.c

san: utf-8-lineseparator.san

clean:
	rm -f utf-8-lineseparator utf-8-lineseparator.san

.PHONY: clean
