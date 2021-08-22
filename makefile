SH	= /bin/bash

CC	= gcc
CFLAGS	= -Wall -Wextra -fanalyzer -g -std=gnu11
CLIBS	= -lm

OBJ	= main.o
TARGET	= ecalc

.PHONY=clean

%.o: %.c
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(CLIBS) $^ -o $@

clean:
	@rm *.o $(TARGET)
