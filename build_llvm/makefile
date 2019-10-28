CC=clang
CFLAGS=-std=gnu11 -pedantic-errors -fdiagnostics-color=always -fno-exceptions -fno-asynchronous-unwind-tables -fstrict-aliasing -g -MD
ALLFLAGS=-O2 -Wall -Wextra -Wpedantic -Weverything -Wno-unused-function -Wno-padded
IFLAGS=-I../include
LFLAGS=-lwinmm -lgdi32 -lkernel32 -lole32 -luuid -ldxgi -ld3d11 -mwindows -municode -lpthread -static-libgcc

all: ../include/pch.h.pch main.exe main.s
../include/pch.h.pch: ../include/pch.h
	$(CC) -c -o $@ $^ -Xclang -emit-pch $(CFLAGS) $(ALLFLAGS) $(IFLAGS) -x c-header
%.s: ../src/%.c
	$(CC) -include-pch ../include/pch.h.pch -S -o $@ $^ $(CFLAGS) $(ALLFLAGS) $(IFLAGS)
%.o: %.s
	$(CC) -c -o $@ $^
main.exe: main.o
	$(CC) -o $@ $^ $(LFLAGS)