CFLAGS = -Wall -Wextra -g -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference -Wpointer-arith -Wcast-qual -Wcast-align=strict #-O2 end of program
CLANGFLAGS = -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align -fsyntax-only #error de linkage avec -lelf sur cette écriture

all : copy isos_inject codeInjection clang backup


copy : 
	rm date
	cp backup/date .

isos_inject: isos_inject.c
	gcc-10 $(CFLAGS) -o $@ $^ -lelf

codeInjection: codeInjection.s
	nasm codeInjection.s -f bin

codeInjectionEntry: codeInjectionEntry.s
	nasm codeInjectionEntry.s -f bin

clang: isos_inject.c
	clang-12 $(CLANGFLAGS) isos_inject.c

analyze: isos_inject.c
	gcc-10 -fanalyzer -Wanalyzer-too-complex isos_inject.c -lelf -o isos_inject

clang-tidy: isos_inject.c
	clang-tidy isos_inject.c

testEntry: isos_inject codeInjectionEntry #Test with modifying entry point
	rm date
	cp backup/date .
	./isos_inject -r date -b codeInjectionEntry -c nouveauNom -a 6300000 -e

test: isos_inject codeInjection #Test with got entry only
	rm date
	cp backup/date .
	./isos_inject -r date -b codeInjection -c nouveauNom -a 6300000

clean:
	rm -f *~ *.o isos_inject codeInjection a.out codeInjectionEntry
