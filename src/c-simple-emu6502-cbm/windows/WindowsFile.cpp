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
	closed = false;
	firstFile = false;
	findFileData = 0;
	hFind = INVALID_HANDLE_VALUE;
	_name = 0;
	fd = -1;
	if (filename == 0)
		return;
	_name = _strdup(filename);
	auto result = _sopen_s(&fd, filename, mode == FILE_READ ? _O_RDONLY | _O_BINARY : _O_RDWR | _O_CREAT | _O_BINARY, SH_DENYNO, _S_IREAD | _S_IWRITE);
	if (result == 0 || mode != FILE_READ || errno != EACCES)
		return;
	findFileData = (WIN32_FIND_DATAA*)malloc(sizeof(WIN32_FIND_DATAA));
	if (findFileData == 0)
		return;
	memset(findFileData, 0, sizeof(WIN32_FIND_DATAA));
	auto size = strlen(_name) + 5;
	char* path = new char[size];
	if (path == 0)
		throw "out of memory";
	sprintf_s(path, size, "%s/*.*", filename);
	hFind = FindFirstFileA(path, findFileData);
	delete[] path;
	if (hFind == INVALID_HANDLE_VALUE)
		return;
	firstFile = true;
}

File::~File()
{
	//if (!closed)
	//	close();
	// TODO: implement non-default copy constructor to duplicate memory usage and handles ???
}

const char* File::name()
{
	char* filenameOnly = _name;
	char* p = strrchr(_name, '/');
	bool hasPath = (p != 0);
	if (hasPath)
		filenameOnly = &p[1];
	return filenameOnly;
}

bool File::isDirectory(void) const
{
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	return ((findFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

File File::openNextFile()
{
	if (firstFile || FindNextFileA(hFind, findFileData)) {
		firstFile = false;
		auto size = strlen(_name) + 1 + strlen(findFileData->cFileName) + 1;
		char* path = new char[size];
		if (path == 0)
			return File(0);
		sprintf_s(path, size, "%s/%s", _name, findFileData->cFileName);
		File f = File(path);
		delete[] path;
		return f;
	}
	else
		return File(0);
}

void File::read(unsigned char* buffer, int size)
{
	if (fd == -1)
		return;
	auto bytes = _read(fd, buffer, size);
}

void File::write(const unsigned char* buffer, int size)
{
	if (fd == -1)
		return;
	_write(fd, buffer, size);
}

void File::seek(int offset)
{
	if (fd == -1)
		return;
	_lseek(fd, offset, SEEK_SET);
}

void File::close()
{
	if (fd != -1) {
		_close(fd);
		fd = -1;
	}
	if (_name != 0) {
		free(_name);
		_name = 0;
	}
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
	if (findFileData != 0) {
		free(findFileData);
		findFileData = 0;
	}
	closed = true;
}

int File::size()
{
	if (isDirectory())
		return 0;
	if (fd == -1)
		return 0;
	long save = _lseek(fd, 0, SEEK_CUR);
	long size = _lseek(fd, 0, SEEK_END);
	_lseek(fd, save, SEEK_SET);
	return size;
}

File FS::open(const char* filename, FILEMODE mode)
{
	while (filename[0] == '/')
		++filename;
	return File(filename, mode);
}

bool FS::mkdir(const char* path)
{
	if (path == 0)
		return false;
	while (*path == '/')
		++path;
	return CreateDirectoryA(path, NULL);
}

FS SD;
#endif // _WINDOWS
