# Makefile
# for cygwin, Linux, UNIX, etc.

c-simple-emu6502-cbm.exe: obj/main.o obj/emuc64.o obj/cbmconsole.o obj/emu6502.o
	$(CC) -o c-simple-emu6502-cbm.exe obj/main.o obj/emuc64.o obj/cbmconsole.o obj/emu6502.o

obj/main.o: main.c emuc64.h emu6502.h
	mkdir -p obj
	$(CC) -o obj/main.o -c main.c

obj/emuc64.o: emuc64.c emuc64.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CC) -o obj/emuc64.o -c emuc64.c

obj/cbmconsole.o: cbmconsole.c cbmconsole.h 
	mkdir -p obj
	$(CC) -o obj/cbmconsole.o -c cbmconsole.c

obj/emu6502.o: emu6502.c emu6502.h
	mkdir -p obj
	$(CC) -o obj/emu6502.o -c emu6502.c

clean:
	rm -f emu6502.exe obj/*
