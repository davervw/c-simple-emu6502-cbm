# Makefile
# for cygwin, Linux, UNIX, etc.

c-simple-emu6502-cbm: obj/main.o obj/emuc64.o obj/emucbm.o obj/emuc128.o obj/emupet.o obj/emuvic20.o obj/emuted.o obj/emud64.o obj/cbmconsole.o obj/emu6502.o obj/emutest.o
	$(CXX) -O3 -std=c++11 -o c-simple-emu6502-cbm obj/main.o obj/emucbm.o obj/emuc64.o obj/emuc128.o obj/emupet.o obj/emuvic20.o obj/emuted.o obj/emud64.o obj/cbmconsole.o obj/emu6502.o obj/emutest.o

obj/main.o: main.cpp emuc64.h emu6502.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/main.o -c main.cpp

obj/emuc64.o: emuc64.cpp emuc64.h emucbm.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emuc64.o -c emuc64.cpp

obj/emucbm.o: emucbm.cpp emucbm.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emucbm.o -c emucbm.cpp

obj/emuc128.o: emuc128.cpp emuc128.h emucbm.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emuc128.o -c emuc128.cpp

obj/emupet.o: emupet.cpp emupet.h emucbm.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emupet.o -c emupet.cpp

obj/emuvic20.o: emuvic20.cpp emuvic20.h emucbm.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emuvic20.o -c emuvic20.cpp

obj/emuted.o: emuted.cpp emuted.h emucbm.h emu6502.h cbmconsole.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emuted.o -c emuted.cpp

obj/emud64.o: emud64.cpp emud64.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emud64.o -c emud64.cpp

obj/cbmconsole.o: cbmconsole.cpp cbmconsole.h 
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/cbmconsole.o -c cbmconsole.cpp

obj/emu6502.o: emu6502.cpp emu6502.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emu6502.o -c emu6502.cpp

obj/emutest.o: emutest.cpp emutest.h
	mkdir -p obj
	$(CXX) -O3 -std=c++11 -o obj/emutest.o -c emutest.cpp

clean:
	rm -f c-simple-emu6502-cbm obj/*
