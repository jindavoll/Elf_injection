CFLAGS = -Wall -Wextra -g -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference -Wpointer-arith -Wcast-qual -Wcast-align=strict #-O2 end of program
CLANGFLAGS = -fsyntax-only -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align

all : isos_inject injectedBinary

isos_inject: isos_inject.c
	gcc $(CFLAGS) -o $@ $^ -lelf

injectedBinary: injectedBinary.c
	$(CC) $(CFLAGS) -c $<

clang: isos_inject.c
	clang  $(CLANGFLAGS) isos_inject.c

analyze: isos_inject.c
	gcc -fanalyzer -Wanalyzer-too-complex isos_inject.c -lelf

clean:
	rm -f *~ *.o isos_inject injectedBinary