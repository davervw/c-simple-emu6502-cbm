// c-simple-emu-cbm.cs - main()
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright(c) 2020 by David R.Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//
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
#include "emuc64.h"
#include "emu6502.h"

int main(int argc, char* argv[])
{
	fprintf(stderr, "\n");
	fprintf(stderr, "c-simple-emu-cbm version 1.3\n");
	fprintf(stderr, "Copyright (c) 2020 by David R. Van Wagner\n");
	fprintf(stderr, "MIT License\n");
	fprintf(stderr, "github.com/davervw\n");
	fprintf(stderr, "\n");
	if (argc > 1)
		StartupPRG = argv[1];
	C64_Init(64*1024, "basic", "chargen", "kernal");
	ResetRun(ExecutePatch);
	return 0;
}
