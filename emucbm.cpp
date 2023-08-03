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

const char* EmuCBM::StartupPRG = 0;

extern "C" void* D64_CreateOrLoad(const char* filename);
extern "C" int D64_GetDirectoryProgram(void* disk, unsigned char* buffer, int* p_ret_file_len);
extern "C" void D64_ReadFileByName(void* disk, unsigned char* filename, unsigned char* buffer, int* p_ret_file_len);
extern "C" int D64_FileSave(void* disk, char* filename, unsigned char* buffer, int buffer_len);
static void* disk = NULL;

EmuCBM::EmuCBM(Memory* mem) : Emu6502(mem)
{
	FileName = NULL;
	FileNum = 0;
	FileDev = 0;
	FileSec = 0;
	FileVerify = false;
	FileAddr = 0;

	LOAD_TRAP = -1;
}

EmuCBM::~EmuCBM()
{
}

bool EmuCBM::ExecutePatch()
{
    if (PC == 0xFFD2) // CHROUT
    {
        CBM_Console_WriteChar((char)A, false);
		// fall through to regular routine to draw character in screen memory too
    }
    else if (PC == 0xFFCF) // CHRIN
    {
        SetA(CBM_Console_ReadChar());
        C = false;

        return ExecuteRTS();
    }
    else if (PC == 0xFFE4) // GETIN
    {
        //BASIC TEST:
        //10 GET K$ : REM GETIN
        //20 IF K$<> "" THEN PRINT ASC(K$)
        //25 IF K$= "Q" THEN END
        //30 GOTO 10

        C = false;
        //SetA(CBM_Console_GetIn());
        SetA(CBM_Console_ReadChar());
        if (A != 0)
            X = A; // observed this side effect from tracing code, so replicating

        return ExecuteRTS();
    }
    //else if (PC == 0xFFE1) // STOP
    //{
    //    Z = CBM_Console_CheckStop();

    //    return ExecuteRTS();
    //}
    else if (PC == 0xFFBA) // SETLFS
    {
        FileNum = A;
        FileDev = X;
        FileSec = Y;
        //System.Diagnostics.Debug.WriteLine(string.Format("SETLFS {0},{1},{2}", FileNum, FileDev, FileSec));
    }
    else if (PC == 0xFFBD) // SETNAM
    {
        static char name[256];
        ushort addr = (ushort)(X + (Y << 8));
        for (int i = 0; i < A; ++i)
            name[i] = (char)GetMemory((ushort)(addr + i));
        name[A] = 0;
        //System.Diagnostics.Debug.WriteLine(string.Format("SETNAM {0}", name.ToString()));
        FileName = name;
    }
    else if (PC == 0xFFD5) // LOAD
    {
        FileAddr = (ushort)(X + (Y << 8));
        //string op;
        //if (A == 0)
        //    op = "LOAD";
        //else if (A == 1)
        //    op = "VERIFY";
        //else
        //    op = string.Format("LOAD (A={0}) ???", A);
        FileVerify = (A == 1);

        ExecuteRTS();

        if (A == 0 || A == 1)
        {
            LOAD_TRAP = PC;

            // Set success
            C = false;
        }
        else
        {
            SetA(14); // ILLEGAL QUANTITY message
            C = true; // failure
        }

        return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
    }
    else if (PC == 0xFFD8) // SAVE
    {
        ushort addr1 = (ushort)(GetMemory(A) + (GetMemory((ushort)(A + 1)) << 8));
        ushort addr2 = (ushort)(X + (Y << 8));
        //System.Diagnostics.Debug.WriteLine(string.Format("SAVE {0:X4}-{1:X4}", addr1, addr2));

        // Set success
        C = !FileSave(FileName, addr1, addr2);

        return ExecuteRTS();
    }
    return false;
}

bool EmuCBM::ExecuteRTS()
{
    byte bytes;
    RTS(&PC, &bytes);
    return true; // return value for ExecutePatch so will reloop execution to allow berakpoint/trace/ExecutePatch/etc.
}

bool EmuCBM::ExecuteJSR(ushort addr)
{
    ushort retaddr = (PC - 1) & 0xFFFF;
    Push(HI(retaddr));
    Push(LO(retaddr));
    PC = addr;
    return true; // return value for ExecutePatch so will reloop execution to allow berakpoint/trace/ExecutePatch/etc.
}

void EmuCBM::File_ReadAllBytes(byte* bytes, unsigned int size, const char* filename)
{
	int file;
#ifdef WIN32	
	_set_errno(0);
	_sopen_s(&file, filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
#else
	file = open(filename, O_RDONLY);
#endif	
	if (file < 0)
	{
		char buffer[40];
#ifdef WIN32
		strerror_s(buffer, sizeof(buffer), errno);
#else		
		strerror_r(errno, buffer, sizeof(buffer));
#endif		
		printf("file ""%""s, errno=%d, %s", filename, errno, buffer);
		exit(1);
	}
#ifdef WIN32
	_read(file, bytes, size);
	_close(file);
#else
	read(file, bytes, size);
	close(file);
#endif	
}

static byte* OpenRead(const char* filename, int* p_ret_file_len)
{
	static unsigned char buffer[65536]; // TODO: get actual file size

	if (disk == 0)
	{
		return (byte*)NULL;
		*p_ret_file_len = 0;
	}

	if (filename != NULL && filename[0] == '$' && filename[1] == '\0')
	{
		*p_ret_file_len = sizeof(buffer);
		if (D64_GetDirectoryProgram(disk, buffer, p_ret_file_len))
			return &buffer[0];
		else
		{
			return (byte*)NULL;
			*p_ret_file_len = 0;
		}
	}
	else
	{
		*p_ret_file_len = sizeof(buffer);
		D64_ReadFileByName(disk, (unsigned char*)filename, buffer, p_ret_file_len);
		return buffer;
	}
}

// returns success
bool EmuCBM::FileLoad(byte* p_err)
{
	bool startup = (StartupPRG != 0);
	ushort addr = FileAddr;
	bool success = true;
	byte err = 0;
	const char* filename = (StartupPRG != 0) ? StartupPRG : FileName;
	int file_len;
	byte* bytes = OpenRead(filename, &file_len);
	ushort si = 0;
	if (bytes == 0 || file_len == 0) {
		*p_err = 4; // FILE NOT FOUND
		success = false;
		FileAddr = addr;
		return success;
	}

	byte lo = bytes[si++];
	byte hi = bytes[si++];
	if (startup) {
		if (lo == 1)
			FileSec = 0;
		else
			FileSec = 1;
	}
	if (FileSec == 1) // use address in file? yes-use, no-ignore
		addr = lo | (hi << 8); // use address specified in file
	while (success) {
		if (si < file_len) {
			byte i = bytes[si++];
			if (FileVerify) {
				if (GetMemory(addr) != i) {
					*p_err = 28; // VERIFY
					success = false;
				}
			}
			else
				SetMemory(addr, i);
			++addr;
		}
		else
			break; // end of file
	}
	FileAddr = addr;
	return success;
}

bool EmuCBM::FileSave(const char* filename, ushort addr1, ushort addr2)
{
	if (filename == NULL || *filename == 0)
		filename = "FILENAME";
	if (disk == 0)
		return false;
	int len = addr2 - addr1 + 2;
	unsigned char* bytes = (unsigned char*)malloc(len);
	if (bytes == 0 || len < 2)
		return false;
	bytes[0] = LO(addr1);
	bytes[1] = HI(addr1);
	for (int i = 0; i < len - 2; ++i)
		bytes[i + 2] = GetMemory(addr1 + i);
	int result = D64_FileSave(disk, (char*)filename, bytes, len);
	free(bytes);
	return result;
}

bool EmuCBM::LoadStartupPrg()
{
	bool result;
	byte err;
	if (disk == 0)
		disk = D64_CreateOrLoad(FileName);
	result = FileLoad(&err);
	if (!result)
		return false;
	else
		return FileSec == 0 ? true : false; // relative is BASIC, absolute is ML
}
