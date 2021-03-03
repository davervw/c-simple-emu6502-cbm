# Makefile
# for cygwin, Linux, UNIX, etc.

c-simple-emu6502-cbm.exe: obj/main.o obj/emuc64.o obj/emud64.o obj/cbmconsole.o obj/emu6502.o
	$(CXX) -O9 -o c-simple-emu6502-cbm.exe obj/main.o obj/emuc64.o obj/emud64.o obj/cbmconsole.o obj/emu6502.o

obj/main.o: main.c emuc64.h emu6502.h
	mkdir -p obj
	$(CC) -O9 -o obj/main.o -c main.c

obj/emuc64.o: emuc64.c emuc64.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CC) -O9 -o obj/emuc64.o -c emuc64.c

obj/emud64.o: emud64.cpp emud64.h
	mkdir -p obj
	$(CC) -O9 -o obj/emud64.o -c emud64.cpp

obj/cbmconsole.o: cbmconsole.c cbmconsole.h 
	mkdir -p obj
	$(CC) -O9 -o obj/cbmconsole.o -c cbmconsole.c

obj/emu6502.o: emu6502.c emu6502.h
	mkdir -p obj
	$(CC) -O9 -o obj/emu6502.o -c emu6502.c

clean:
	rm -f emu6502.exe obj/*
