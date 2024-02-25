CC = gcc
CFLAGS = -Wall -g

OBJS = prototype.o addrinfo.o userinput.o list.o

all: s-talk

s-talk: $(OBJS)
	$(CC) $(CFLAGS) -o s-talk $(OBJS)

prototype.o: prototype.c
	$(CC) $(CFLAGS) -c prototype.c

addrinfo.o: addrinfo.c
	$(CC) $(CFLAGS) -c addrinfo.c

userinput.o: userinput.c
	$(CC) $(CFLAGS) -c userinput.c

list.o: list.o
	$(CC) $(CFLAGS) -c list.o

clean:
	rm -f *.o s-talk