# Makefile for Writing Make Files Example

# *****************************************************
# Variables to control Makefile operation

CC = gcc
CFLAGS = -Wall -g

# ****************************************************
# Targets needed to bring the executable up to date

main: x3d-lib-test.o x3d.o
	$(CC) $(CFLAGS) -o main.out x3d-lib-test.o x3d.o

# The main.o target can be written more simply

main.o: x3d-lib-test.c x3d.h
	$(CC) $(CFLAGS) -c x3d-lib-test.c

x3d.o: x3d.h
