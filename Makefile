CFLAGS = -Wall -Wextra -g -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference -Wpointer-arith -Wcast-qual -Wcast-align=strict #-O2 end of program
CLANGFLAGS = -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align -fsyntax-only #error de linkage avec -lelf sur cette écriture

all : copy isos_inject codeInjection valgrind clang backup


copy : 
	rm date
	cp backup/date .

isos_inject: isos_inject.c
	gcc-10 $(CFLAGS) -o $@ $^ -lelf

codeInjection: codeInjection.s
	nasm codeInjection.s -f bin

clang: isos_inject.c
	clang-12 $(CLANGFLAGS) isos_inject.c

analyze: isos_inject.c
	gcc-10 -fanalyzer -Wanalyzer-too-complex isos_inject.c -lelf -o isos_inject

clang-tidy: isos_inject.c
	clang-tidy isos_inject.c

test: isos_inject codeInjection
	rm date
	cp backup/date .
	./isos_inject -r date -b codeInjection -c nouveauNom -a 6300000

valgrind: isos_inject codeInjection
	rm date
	cp backup/date .
	valgrind ./isos_inject -r date -b codeInjection -c nouveauNom -a 6300000 

test_clang: clang codeInjection 
	rm date
	cp backup/date .

test_analyze: analyze codeInjection
	rm date
	cp backup/date .

clean:
	rm -f *~ *.o isos_inject codeInjection a.out
