CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
TARGET = umon
SRC = umon.c
LIBS = -lm

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
