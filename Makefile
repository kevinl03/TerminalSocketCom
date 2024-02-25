CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# List of source files (excluding list.c)
SOURCES = prototype.c addrinfo.c userinput.c list.o

# Output executable
TARGET = proto

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)
