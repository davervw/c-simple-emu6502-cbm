#include "emutest.h"
#include "emu6502.h"

void start(bool& shuttingDown)
{
	Emu6502* system = new EmuTest();
	system->ResetRun();
	delete system;
}