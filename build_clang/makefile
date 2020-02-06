CC=clang
CFLAGS=-std=gnu11 -fdiagnostics-color=always -fno-exceptions -fno-asynchronous-unwind-tables -fno-stack-protector -fstrict-aliasing -fno-rtti -m64 -ffast-math -MD
ALLFLAGS=-O2 -g -Wall -Wextra -Wpedantic -Weverything -mcmodel=small
IFLAGS=-I../include
LFLAGS=-lwinmm -lgdi32 -lkernel32 -lole32 -luuid -ldxgi -ld3d11 -mwindows -municode -lpthread -ladvapi32 -static-libgcc

all: ../include/pch.h.pch main.exe main.s ../hlsl/vertex_shader.cso ../hlsl/pixel_shader.cso main.dump
../include/pch.h.pch: ../include/pch.h
	$(CC) -c -o $@ $^ -Xclang -emit-pch $(CFLAGS) $(ALLFLAGS) $(IFLAGS) -x c-header
%.s: ../src/%.c
	$(CC) -include-pch ../include/pch.h.pch -S -o $@ $^ $(CFLAGS) $(ALLFLAGS) $(IFLAGS)
%.o: %.s
	$(CC) -c -o $@ $^
main.exe: main.o
	$(CC) -o $@ $^ $(LFLAGS)
../hlsl/vertex_shader.cso: ../src/hlsl/vertex_shader.hlsl
	../fxc.exe /T vs_4_1 /E main /O3 /Zi ../src/hlsl/vertex_shader.hlsl /Fc ../hlsl/vertex_shader.asm /Fo ../hlsl/vertex_shader.cso
../hlsl/pixel_shader.cso: ../src/hlsl/pixel_shader.hlsl
	../fxc.exe /T ps_4_1 /E main /O3 /Zi ../src/hlsl/pixel_shader.hlsl /Fc ../hlsl/pixel_shader.asm /Fo ../hlsl/pixel_shader.cso
main.dump: main.exe
	objdump -M intel -d -S $^ > $@