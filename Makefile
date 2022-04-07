CFLAGS = -Wall -Wextra -g -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference -Wpointer-arith -Wcast-qual -Wcast-align=strict #-O2 end of program
CLANGFLAGS = -fsyntax-only -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align

all : copy isos_inject codeInjection valgrind


copy : 
	rm date
	cp backup/date .

isos_inject: isos_inject.c
	gcc $(CFLAGS) -o $@ $^ -lelf

codeInjection: codeInjection.s
	nasm codeInjection.s

clang: isos_inject.c
	clang  $(CLANGFLAGS) isos_inject.c

analyze: isos_inject.c
	gcc-10 -fanalyzer -Wanalyzer-too-complex isos_inject.c -lelf

test: isos_inject codeInjection
	rm date
	cp backup/date .
	./isos_inject -r date -b codeInjection -c nouveauNom -a 6300000

valgrind: isos_inject codeInjection
	rm date
	cp backup/date .
	valgrind ./isos_inject -r date -b codeInjection -c nouveauNom -a 6300000

clean:
	rm -f *~ *.o isos_inject codeInjection