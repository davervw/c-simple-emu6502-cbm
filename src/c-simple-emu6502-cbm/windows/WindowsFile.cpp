// WindowsFile.cpp - Arduino File compatibility layer for Windows
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Unified Emulator for M5Stack/Teensy/ESP32 LCDs and Windows
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

#ifdef _WINDOWS
#include "framework.h"
#include "WindowsFile.h"
#include <corecrt_io.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>


File::File(const char* filename, FILEMODE mode)
{
	_sopen_s(& fd, filename, mode == FILE_READ ? _O_RDONLY | _O_BINARY : _O_RDWR | _O_CREAT | _O_BINARY, SH_DENYNO, _S_IREAD | _S_IWRITE);
}

void File::read(unsigned char* buffer, int size)
{
	auto bytes = _read(fd, buffer, size);
}

void File::write(const unsigned char* buffer, int size)
{
	_write(fd, buffer, size);
}

void File::seek(int offset)
{
	_lseek(fd, offset, SEEK_SET);
}

void File::close()
{
	_close(fd);
	fd = -1;
}

int File::size()
{
	long save = _lseek(fd, 0, SEEK_CUR);
	long size = _lseek(fd, 0, SEEK_END);
	_lseek(fd, save, SEEK_SET);
	return size;
}

File FS::open(const char* filename, FILEMODE mode)
{
	if (filename[0] == '/')
		++filename;
	return File(filename, mode);
}

bool FS::mkdir(const char* path)
{
	if (path == 0)
		return false;
	if (*path == '/')
		++path;
	return CreateDirectoryA(path, NULL);
}

FS SD;
#endif // _WINDOWS