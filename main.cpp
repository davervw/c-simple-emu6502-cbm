// main.cpp - main()
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright (c) 2024 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "emuc64.h"
#include "emuc128.h"
#include "emuted.h"
#include "emuvic20.h"
#include "emupet.h"
#include "emutest.h"
#include "emumin.h"
#include <string.h>

int main_go_num = 0;

int fileExists(const char* filename)
{
#ifdef WINDOWS
	FILE* fp;
	fopen_s(&fp, filename, "rb");
#else
	FILE* fp = fopen(filename, "rb");
#endif
	int exists = (fp != 0);
	if (exists)
		fclose(fp);
	return exists;
}

int main(int argc, char* argv[])
{
	fprintf(stderr, "\n");
	fprintf(stderr, "c-simple-emu-cbm version 1.7\n");
	fprintf(stderr, "Copyright (c) 2024 by David R. Van Wagner\n");
	fprintf(stderr, "MIT License\n");
	fprintf(stderr, "github.com/davervw\n");
	fprintf(stderr, "\n");
	for (int i = 1; i < argc; ++i)
	{
		if (fileExists(argv[i]))
			EmuCBM::StartupPRG = argv[i];
		else
			main_go_num = atoi(argv[i]);
	}

	while (true)
	{
		Emu6502* emu;

		if (main_go_num == 128)
			emu = new EmuC128();
		else if (main_go_num == 4)
			emu = new EmuTed(64);
		else if (main_go_num == 16)
			emu = new EmuTed(16);
		else if (main_go_num == 20)
			emu = new EmuVic20(5);
		else if (main_go_num == 2001)
			emu = new EmuPET(32);
		else if (main_go_num == -1)
			emu = new EmuTest(EmuCBM::StartupPRG);
		else if (main_go_num == 1)
		{
			char buffer[256];
			bool unknown_filename = (EmuCBM::StartupPRG == 0 || *EmuCBM::StartupPRG == 0);
			if (unknown_filename)
			{
				puts("Minimum ROM Filename? ");
				EmuCBM::StartupPRG = fgets(buffer, sizeof(buffer), stdin);
				auto len = strlen(buffer);
				if (len > 0 && buffer[len - 1] == '\n')
					buffer[len - 1] = 0;
			}

			emu = new EmuMinimum(EmuCBM::StartupPRG, 0xFFF8, false);
		}
		else
			emu = new EmuC64(64 * 1024);

		emu->ResetRun();
		delete emu;

		if (main_go_num == -1)
			break;
	}

	return 0;
}
