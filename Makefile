CFLAGS = -Wall -Wextra -g

all : isos_inject injectedBinary

isos_inject: isos_inject.c
	$(CC) $(CFLAGS) -o $@ $^ -lelf

injectedBinary: injectedBinary.c
	$(CC) $(CFLAGS) -c $<

clang: isos_inject.c
	clang isos_inject.c -lelf

clean:
	rm -f *~ *.o isos_inject injectedBinary