CC=gcc
CFLAGS=-std=gnu11 -pedantic-errors -fdiagnostics-color=always -fno-exceptions -fno-asynchronous-unwind-tables -fstrict-aliasing -g -MD
ALLFLAGS=-O2 -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-padded
IFLAGS=-I../include
LFLAGS=-lwinmm -lgdi32 -lkernel32 -lole32 -luuid -ldxgi -ld3d11 -mwindows -municode -lpthread -static-libgcc

all: ../include/pch.h.gch main.exe main.s
../include/pch.h.gch: ../include/pch.h
	$(CC) -c -o $@ -x c-header $^ $(CFLAGS) $(ALLFLAGS) $(IFLAGS)
%.s: ../src/%.c
	$(CC) -S -o $@ $^ $(CFLAGS) $(ALLFLAGS) $(IFLAGS)
%.o: %.s
	$(CC) -c -o $@ $^
main.exe: main.o
	$(CC) -o $@ $^ $(LFLAGS)