#include "emucbm.h"

#include "cbmconsole.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <share.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

EmuCBM::EmuCBM(Memory* mem) : Emu6502(mem)
{
}

bool EmuCBM::ExecutePatch()
{
	return false;
}
