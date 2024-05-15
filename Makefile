CC = gcc
CFLAGS = -Wall -Werror -Wextra
TARGET = lab11vndN3250
SRCS = lab11vndN3250.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
