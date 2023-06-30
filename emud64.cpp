////////////////////////////////////////////////////////////////////////////////
//
// emud64.cpp - Class EmuD64
//   1541 Disk Image Driver - access files, directory from disk image
//
// MIT License
//
// Copyright (c) 2023 by David R. Van Wagner
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

// .D64 File format documented at https://vice-emu.sourceforge.io/vice_17.html#SEC345
// and http://unusedino.de/ec64/technical/formats/d64.html

// ported from EmuD64 class at https://github.com/davervw/ts-emu-c64/blob/master/c64-6502.ts
// and added write capability

#include "M5Core.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "emud64.h"

//extern SDClass SD;

//#define snprintf sprintf_s

static void strcpy_s(char* dest, size_t size, const char* src)
{
	strncpy(dest, src, size);
}

static void strcat_s(char* dest, size_t size, const char* src)
{
	strncat(dest, src, size);
}

static const int sectors_per_track[36/*n_tracks+1*/] =
{
    0, // there is no track 0
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    19, 19, 19, 19, 19, 19, 19,
    18, 18, 18, 18, 18, 18,
    17, 17, 17, 17, 17,
};

static const int file_sector_interleave = 10;
static const int dir_sector_interleave = 3;

EmuD64::EmuD64(const char* filename_d64)
{
    bytes = 0;
    size_t len = strlen(filename_d64) + 1;
    this->filename_d64 = new char[len];
    strcpy_s(this->filename_d64, len, filename_d64);
    track_dirty = new bool[n_tracks + 1]; // +1 because this index is one based
    LoadFromFilenameOrCreate();
}

EmuD64::~EmuD64()
{
    FlushDisk();
    delete[] bytes;
    delete[] filename_d64;
    delete[] track_dirty;
}

void EmuD64::InitializeData(unsigned char* disk_name, unsigned char* id)
{
    // wipe disk
    memset(bytes, 0, bytes_per_disk);

    // initialize BAM
    int i = GetSectorOffset(bam_track, bam_sector);
    bytes[i++] = dir_track;
    bytes[i++] = dir_sector;
    bytes[i++] = 0x41; // 'A' - Disk DOS version type
    bytes[i++] = 0; // unused

    // initalize BAM free space
    for (int track = 1; track <= n_tracks; ++track)
    {
        bytes[i++] = sectors_per_track[track];
        bytes[i++] = 0xFF;
        bytes[i++] = 0xFF;
        bytes[i++] = (1 << (sectors_per_track[track] - 16)) - 1;
    }

    int disk_name_len = (int)strlen((char*)disk_name);
    for (int j = 0; j < disk_name_size; ++j)
        bytes[i++] = (j < disk_name_len) ? disk_name[j] : 0xA0;

    bytes[i++] = 0xA0;
    bytes[i++] = 0xA0;

    int disk_id_len = (int)strlen((char*)id);
    for (int j = 0; j < disk_id_size; ++j)
        bytes[i++] = (j < disk_id_len) ? id[j] : 0xA0;

    bytes[i++] = 0xA0;

    // DOS type
    bytes[i++] = 0x32; // '2'
    bytes[i++] = 0x41; // 'A'

    // filled
    bytes[i++] = 0xA0;
    bytes[i++] = 0xA0;
    bytes[i++] = 0xA0;
    bytes[i++] = 0xA0;

    // initialize first directory sector
    i = GetSectorOffset(dir_track, dir_sector);
    for (int j = 0; j < dir_entries_per_sector; ++j)
    {
        for (int k = 0; k < EmuD64::DirStruct::dir_name_size; ++k)
            bytes[i + j * dir_entry_size + 5 + k] = 0xA0; // filename padding
    }

    // reserve first two sectors from directory track
    AllocBlock(true);
    AllocBlock(true);
}

static void show_exception(const char* message)
{
  while (!M5Serial); // wait for serial terminal connected
  M5Serial.println(message);
  while(1);
}

int EmuD64::GetSectorOffset(int track, int sector)
{
    if (track < 1 || track > n_tracks)
    {
        char exception[80];
        snprintf(exception, sizeof(exception), "track %d out of range, should be 1 to %d", track, n_tracks);
        show_exception(exception);
    }
    if (sector < 0 || sector >= sectors_per_track[track])
    {
        char exception[80];
        snprintf(exception, sizeof(exception), "sector %d out of range, should be 0 to %d for track %d", sector, sectors_per_track[track], track);
        show_exception(exception);
    }
    int offset = 0;
    for (int t = 1; t < track; ++t)
        offset += sectors_per_track[t] * bytes_per_sector;
    offset += sector * bytes_per_sector;
    return offset;
}

EmuD64::DirStruct::DirStruct()
{
    next_track = 0;
    next_sector = 0;
    file_type = FileType::DEL;
    file_track = 0;
    file_sector = 0;
    for (int i = 0; i < EmuD64::DirStruct::dir_name_size; ++i)
        filename[i] = 0xA0;
    rel_track = 0;
    rel_sector = 0;
    rel_length = 0;
    for (int i = 0; i < dir_unused_size; ++i)
        unused[i] = 0;
    n_sectors = 0;
}

void EmuD64::DirStruct::Read(EmuD64* d64, int track, int sector, int n)
{
    if (n < 0 || n >= d64->dir_entries_per_sector)
    {
        char exception[80];
        snprintf(exception, sizeof(exception), "directory index %d out of range, expected 0 to %d", n, d64->dir_entries_per_sector - 1);
        show_exception(exception);
    }
    int i = d64->GetSectorOffset(track, sector) + d64->dir_entry_size * n;
    ReadData(d64, d64->bytes, i);
}

void EmuD64::DirStruct::ReadData(EmuD64* d64, unsigned char* data, int offset)
{
    int save_offset = offset;
    next_track = data[offset++];
    next_sector = data[offset++];
    file_type = data[offset++];
    file_track = data[offset++];
    file_sector = data[offset++];
    for (int i = 0; i < dir_name_size; ++i)
        filename[i] = data[offset++];
    rel_track = data[offset++];
    rel_sector = data[offset++];
    rel_length = data[offset++];
    for (int i = 0; i < dir_unused_size; ++i)
        unused[i] = data[offset++];
    int lo = data[offset++];
    int hi = data[offset++];
    n_sectors = (lo + (hi << 8));
    if (offset != save_offset + d64->dir_entry_size)
    {
        char exception[80];
        snprintf(exception, sizeof(exception), "internal error, expected to read %d  bytes for directory entry, but read %d bytes", d64->dir_entry_size, offset - save_offset);
        show_exception(exception);
    }
}

static struct DirEntryLocation {
    unsigned char filename[EmuD64::DirStruct::dir_name_size];
    bool found_name;

    // found data
    int track;
    int sector;
    int entry;
    int next_track;
    int next_sector;

    // first DEL data
    int empty_track;
    int empty_sector;
    int empty_entry;
    int empty_next_track;
    int empty_next_sector;
} dir_entry_location;

// gather various track/sector/entry location to help us store a new directory entry, or overwrite an existing directory entry
static bool DirectoryEntryLocator(EmuD64* d64, EmuD64::DirStruct* dir, int n, bool last, void*& context)
{
    dir_entry_location.entry = n % EmuD64::dir_entries_per_sector; // track which entry in sector

    if (dir_entry_location.entry == 0) // next entries are only valid every 8 records
    {
        if (n == 0)
        {
            // if first, we know exactly where we are
            dir_entry_location.track = EmuD64::dir_track;
            dir_entry_location.sector = EmuD64::dir_sector;

            // make sure empty is cleared
            dir_entry_location.empty_track = 0;
            dir_entry_location.empty_sector = 0;
            dir_entry_location.empty_entry = 0;
            dir_entry_location.empty_next_track = 0;
            dir_entry_location.empty_next_sector = 0;
        }
        else
        {
            // we know where we are by next values from where we've been
            dir_entry_location.track = dir_entry_location.next_track;
            dir_entry_location.sector = dir_entry_location.next_sector;
        }
        // remember where we're going
        dir_entry_location.next_track = dir->next_track;
        dir_entry_location.next_sector = dir->next_sector;
    }

    if ((dir->file_type & 7) == EmuD64::DirStruct::FileType::DEL
        && dir_entry_location.empty_track == 0)
    {
        dir_entry_location.empty_track = dir_entry_location.track;
        dir_entry_location.empty_sector = dir_entry_location.sector;
        dir_entry_location.empty_entry = dir_entry_location.entry;
        dir_entry_location.empty_next_track = dir_entry_location.next_track;
        dir_entry_location.empty_next_sector = dir_entry_location.next_sector;
    }
    else if ((dir->file_type & 7) == EmuD64::DirStruct::FileType::PRG
        && memcmp(dir->filename, dir_entry_location.filename, EmuD64::DirStruct::dir_name_size) == 0)
    {
        dir_entry_location.found_name = true;
    }
    else
        dir_entry_location.found_name = false;

    return !dir_entry_location.found_name; // return false when found so we stop looking
}

static bool FindDirectoryEntry(EmuD64* d64, EmuD64::DirStruct* dir)
{
    memset(&dir_entry_location, 0, sizeof(dir_entry_location));
    memcpy(&dir_entry_location.filename, dir->filename, EmuD64::DirStruct::dir_name_size);
    void* context = 0;
    d64->WalkDirectory(DirectoryEntryLocator, context); // TODO: consider using context instead of static dir_entry_location
    return dir_entry_location.found_name;
}

bool EmuD64::FindOrAllocDirectoryEntry(EmuD64::DirStruct* dir, int& track, int& sector, int& entry)
{
    if (FindDirectoryEntry(this, dir))
    {
        track = dir_entry_location.track;
        sector = dir_entry_location.sector;
        entry = dir_entry_location.entry;
        if (entry == 0)
        {
            dir->next_track = dir_entry_location.next_track;
            dir->next_sector = dir_entry_location.next_sector;
        }
        return true;
    }
    else if (dir_entry_location.empty_track != 0)
    {
        track = dir_entry_location.empty_track;
        sector = dir_entry_location.empty_sector;
        entry = dir_entry_location.empty_entry;
        if (entry == 0)
        {
            dir->next_track = dir_entry_location.empty_next_track;
            dir->next_sector = dir_entry_location.empty_next_sector;
        }
        return true;
    }
    else
    {
        BlockStruct block = AllocBlock(true);
        if (block.track == 0)
        {
            track = 0;
            sector = 0;
            entry = 0;
            return false;
        }
        track = block.track;
        sector = block.sector;
        entry = 0;

        // initialize directory sector
        int i = GetSectorOffset(track, sector);
        dir->next_track = 0; // will be stored at bytes[i]
        dir->next_sector = 255; // will be stored at bytes[i+1]
        for (int j = 0; j < dir_entries_per_sector; ++j)
        {
            for (int k = 0; k < EmuD64::DirStruct::dir_name_size; ++k)
                bytes[i + j * dir_entry_size + 5 + k] = 0xA0; // filename padding
        }

        // link old last sector to this new sector
        i = GetSectorOffset(dir_entry_location.track, dir_entry_location.sector);
        bytes[i] = track;
        bytes[i + 1] = sector;

        track_dirty[dir_track] = true;

        return true;
    }
}

int EmuD64::DirStruct::Store(EmuD64* d64, int dir_track, int dir_sector, int dir_sector_entry)
{
    if (dir_track != EmuD64::dir_track || dir_sector < 0 || dir_sector > sectors_per_track[dir_track] || dir_sector_entry < 0 || dir_sector_entry > EmuD64::dir_entries_per_sector)
        return 0; // out of range

    file_type |= 0x80; // closed

    int i = d64->GetSectorOffset(dir_track, dir_sector) + dir_sector_entry * EmuD64::dir_entry_size;

    d64->bytes[i + 2] = (int)file_type;
    d64->bytes[i + 3] = file_track;
    d64->bytes[i + 4] = file_sector;

    for (int j = 0; j < sizeof(filename); ++j)
        d64->bytes[i + 5 + j] = filename[j];

    d64->bytes[i + 21] = rel_track;
    d64->bytes[i + 22] = rel_sector;
    d64->bytes[i + 23] = rel_length;

    for (int j = 0; j < sizeof(unused); ++j)
        d64->bytes[i + 24 + j] = unused[j];

    d64->bytes[i + 30] = n_sectors & 0xFF;
    d64->bytes[i + 31] = n_sectors >> 8;

    d64->track_dirty[dir_track] = true;

    return 1;
}

char EmuD64::DirStruct::PrintableChar(unsigned char c)
{
    /*if (c == '^')
        return '↑';
    else if (c == '_')
        return '←';
    else*/ if (c == '[' || c == ']') // brackets
        return c;
    else if (c >= 32 && c <= 64) // punctuation
        return c;
    else if (c >= 'A' && c <= 'Z') // uppercase or lowercase
        return c; // uppercase
    else if (c >= 'a' && c <= 'z') // lowercase or graphics
        return c - 'a' + 'A';
//    else if (c == '\\')
//        return '£';
    else if (c == 0xA0)
        return ' ';
    else
        return '?'; // not printable
}

void EmuD64::DirStruct::getName(unsigned char *name, int size)
{
    memset(name, 0, size);
    int dest = 0;
    for (int i = 0; i < dir_name_size; ++i)
    {
        unsigned char c = filename[i];
        if (c == 0xA0 || dest >= size)
            break;
        name[dest++] = c;
    }
}

const char* EmuD64::DirStruct::FileTypeString()
{
    switch (file_type & 7)
    {
    case FileType::DEL:
        return "DEL";
    case FileType::PRG:
        return "PRG";
    case FileType::REL:
        return "REL";
    case FileType::SEQ:
        return "SEQ";
    case FileType::USR:
        return "USR";
    default:
        return "???";
    }
}

const char* EmuD64::DirStruct::toString()
{
    static char buffer[40];
    memset(buffer, 0, sizeof(buffer));
    char* s = &buffer[0];
    int n_s_len = snprintf(s, sizeof(buffer), "%d", n_sectors);
    s += n_s_len;
    for (int i = 1; i <= 5 - n_s_len; ++i)
        *(s++) = ' ';
    *(s++) = '"';
    int i = 0;
    while (i < dir_name_size && filename[i] != 0xA0)
        *(s++) = PrintableChar(filename[i++]);
    *(s++) = '"';
    while (i < dir_name_size)
        *(s++) = PrintableChar(filename[i++]);
    *(s++) = (file_type & 0x80) ? ' ' : '*';
    *s = 0; // nul terminate
    strcpy_s(s, sizeof(s), FileTypeString());
    return buffer;
}

void EmuD64::WalkDirectory(bool (*dirFn)(EmuD64 *d64, DirStruct *dir, int n, bool last, void*&context), void*& context)
{
    int track = dir_track;
    int sector = dir_sector;
    int next_track = 0;
    int next_sector = 0;
    int n = 0;
    DirStruct *dir = new DirStruct();
    while (true)
    {
        dir->Read(this, track, sector, n % dir_entries_per_sector);
        if ((n % dir_entries_per_sector) == 0)
        {
            next_track = dir->next_track;
            next_sector = dir->next_sector;
        }
        bool last = ((n % dir_entries_per_sector) == dir_entries_per_sector - 1 && next_track == 0);
        bool cont = dirFn(this, dir, n, last, context);
        if (!cont || last)
            break;
        if ((++n % dir_entries_per_sector) == 0)
        {
            track = next_track;
            sector = next_sector;
        }
    }
}

static bool DirectoryCountHandler(EmuD64* d64, EmuD64::DirStruct* dir, int n, bool last, void*& context)
{
    ++*(int*)context;
    return true;
}

int EmuD64::GetDirectoryCount()
{
    int total_count = 0;
    void* context = &total_count;
    WalkDirectory(DirectoryCountHandler, context);
    return total_count;
}

static bool DeletedCountHandler(EmuD64* d64, EmuD64::DirStruct* dir, int n, bool last, void*& context)
{
    int deleted_count = *(int*)context;
    if ((dir->file_type & 7) == EmuD64::DirStruct::FileType::DEL)
    {
        ++deleted_count;
        *(int*)context = deleted_count;
    }
    return true;
}

int EmuD64::GetDeletedCount()
{
    int deleted_count = 0;
    void* context = &deleted_count;
    WalkDirectory(DeletedCountHandler, context);
    return deleted_count;
}

struct DirectoryEntryContext
{
    int n;
    EmuD64::DirStruct* dir;
};

static bool DirectoryEntryHandler(EmuD64* d64, EmuD64::DirStruct* dir, int n, bool last, void*& context)
{
    if (n == ((DirectoryEntryContext*)context)->n)
    {
        ((DirectoryEntryContext*)context)->dir = dir;
        return false;
    }
    return true;
}

EmuD64::DirStruct* EmuD64::DirectoryEntry(int i)
{
    DirectoryEntryContext direntrycontext;
    direntrycontext.n = i;
    direntrycontext.dir = 0;
    void* context = &direntrycontext;
    WalkDirectory(DirectoryEntryHandler, context);
    return direntrycontext.dir;
}

void EmuD64::DiskBAMField(int field_offset, int size, unsigned char* field)
{
    int offset = GetSectorOffset(bam_track, bam_sector);
    for (int i = 0; i < size; ++i)
        field[i] = bytes[offset + field_offset + i];
}

void EmuD64::DiskBAMPrintable(int field_offset, int size, char* field)
{
    DiskBAMField(field_offset, size, (unsigned char*)field);
    for (int i = 0; i < size; ++i)
        field[i] = DirStruct::PrintableChar(field[i]);
}

void EmuD64::DiskName(char* disk_name, int size)
{
    if (size != disk_name_size+1)
    {
        char s[80];
        snprintf(s, sizeof(s), "disk_name size must be %d", disk_name_size+1);
        show_exception(s);
    }
    memset(disk_name, 0, size);
    DiskBAMPrintable(disk_name_offset, disk_name_size, disk_name);
}

void EmuD64::DiskId(char* disk_id, int size)
{
    if (size != disk_id_size+1)
    {
        char s[80];
        snprintf(s, sizeof(s), "disk_id size must be %d", disk_id_size+1);
        show_exception(s);
    }
    memset(disk_id, 0, size);
    DiskBAMPrintable(disk_id_offset, disk_id_size, disk_id);
}

void EmuD64::DiskDosType(char* disk_dos_type, int size)
{
    if (size != disk_dos_type_size+1)
    {
        char s[80];
        snprintf(s, sizeof(s), "disk_dos_type size must be %d", disk_dos_type_size+1);
        show_exception(s);
    }
    memset(disk_dos_type, 0, size);
    DiskBAMPrintable(disk_dos_type_offset, disk_dos_type_size, disk_dos_type);
}

int EmuD64::BlocksFree()
{
    int total_free = 0;
    int offset = GetSectorOffset(bam_track, bam_sector);
    for (int track = 1; track <= n_tracks; ++track)
    {
        int track_free = bytes[offset + track * 4];
        if (track != dir_track && track != bam_track)
            total_free += track_free;
    }
    return total_free;
}

void EmuD64::ReadFileByIndex(int i, unsigned char* data, int &length)
{
    DirStruct* dir = DirectoryEntry(i);
    int file_limit = length;
    length = 0;
    int track = dir->file_track;
    int sector = dir->file_sector;
    if (((dir->file_type) & 7) == (int)(DirStruct::FileType::PRG))
    {
        while (true)
        {
            int offset = GetSectorOffset(track, sector);
            track = bytes[offset];
            sector = bytes[offset + 1];
            int sector_limit = 256;
            if (track == 0)
                sector_limit = sector + 1;
            for (i = 2; i < sector_limit; ++i)
            {
                if (length < file_limit || (data == 0 && file_limit == 0)) // within limit, or not storing
                {
                    if (length < file_limit)
                        data[length] = bytes[offset + i];
                    ++length;
                }
                else
                {
                    length = -1; // read past end of file... return failure
                    return;
                }
            }
            if (track == 0)
                break;
        }
    }
    else
    {
        length = 0;
    }
}

static bool ReadFileByNameHandler(EmuD64* d64, EmuD64::DirStruct* dir, int n, bool last, void*& context)
{
    const char* filename = (char*)context;
    bool isPRG = ((dir->file_type) & 7) == int(EmuD64::DirStruct::FileType::PRG);
    int filename_len = (int)strlen(filename);
    bool doFirst = (filename_len == 1 && filename[0] == '*')
        || (filename_len == 3 && filename[0] == '0' && filename[1] == ':' && filename[2] == '*');
    for (int i = 0; i < EmuD64::DirStruct::dir_name_size; ++i)
    {
        if (((i > 0 && dir->filename[i] == 0xA0 && (filename[i] == 0xA0 || filename[i] == 0)) || doFirst) && isPRG) // end of filename shortcut
            break;
        else if (filename[i] != dir->filename[i]) // no match
            return true; // keep searching
    }
    // full or shortcut match if got here
    static int ret_index;
    ret_index = n;
    context = &ret_index;
    return false; // stop searching
}

void EmuD64::ReadFileByName(const char* filename, unsigned char* bytes, int& length)
{
    char* dupname = strdup(filename);
    void* context = dupname;
    WalkDirectory(ReadFileByNameHandler, context);
    if (context == dupname) // not found
        length = 0;
    else
        ReadFileByIndex(*(int*)context, bytes, length);
    free(dupname);
}

void EmuD64::WriteBlock(BlockStruct block, BlockStruct next_block, unsigned char* data, int data_len, int data_offset)
{
    int disk_offset = GetSectorOffset(block.track, block.sector);
    int size = bytes_per_sector - 2;
    bytes[disk_offset++] = next_block.track;
    bytes[disk_offset++] = next_block.sector;
    if (data_offset + size > data_len) // make sure we don't surpass array limits
        size = data_len - data_offset;
    for (int i = 0; i < size; ++i)
        bytes[disk_offset + i] = data[data_offset + i];
    track_dirty[block.track] = true;
}

EmuD64::BlockStruct EmuD64::AllocBlock(bool directory)
{
    int track;
    int sector = 0;
    bool done = false;

    int offset = GetSectorOffset(bam_track, bam_sector);
    if (directory)
        track = EmuD64::dir_track;
    else
        track = 1;
    do
    {
        int track_bam_offset = offset + track * 4;
        int track_free = bytes[track_bam_offset];
        if (track_free > 0)
        {
            int track_bam = bytes[track_bam_offset + 1] | (bytes[track_bam_offset + 2] << 8) | (bytes[track_bam_offset + 3] << 16);
            for (sector = 0; sector <= sectors_per_track[track]; ++sector) // TODO: 10 sector interleave?
            {
                if (track_bam & (1 << sector)) // found free sector
                {
                    // mark sector used
                    if (sector < 8)
                        bytes[track_bam_offset + 1] ^= (1 << sector);
                    else if (sector < 16)
                        bytes[track_bam_offset + 2] ^= (1 << (sector - 8));
                    else
                        bytes[track_bam_offset + 3] ^= (1 << (sector - 16));

                    // decrement track's free sector count
                    if (bytes[track_bam_offset] > 0)
                        --bytes[track_bam_offset];

                    track_dirty[bam_track] = true;

                    done = true;
                    break;
                }
            }
            if (done)
                break;
        }
        if (++track == EmuD64::dir_track)
            ++track; // skip over directory track
    } while (!directory && track <= EmuD64::n_tracks);

    if (track > n_tracks || (directory && track != EmuD64::dir_track)) 
    {
        track = 0;
        sector = 0;
    }

    BlockStruct block;
    block.track = track;
    block.sector = sector;
    return block;
}

void EmuD64::StoreFileByStruct(DirStruct* dir, unsigned char* data, int data_len)
{
    int n_sectors = (data_len + bytes_per_sector - 2 - 1) / (bytes_per_sector - 2);
    int free = BlocksFree();
    if (free < n_sectors)
    {
        char exception[80];
        unsigned char filename[DirStruct::dir_name_size];
        dir->getName(filename, sizeof(filename));
        snprintf(exception, sizeof(exception), "cannot store file %s (%d blocks) as disk has only %d blocks", filename, n_sectors, free);
        show_exception(exception); // TODO: soft DOS error
    }
    dir->n_sectors = n_sectors;
    int offset = 0;
    if (n_sectors > 0)
    {
        BlockStruct block = AllocBlock(false);
        dir->file_track = block.track;
        dir->file_sector = block.sector;
        int dir_track, dir_sector, dir_entry;
        if (FindOrAllocDirectoryEntry(dir, dir_track, dir_sector, dir_entry)
            && dir->Store(this, dir_track, dir_sector, dir_entry))
        {
            while (n_sectors > 0)
            {
                BlockStruct next_block;
                next_block.track = 0;
                next_block.sector = 0;
                if (--n_sectors == 0) {
                    int size = data_len % (bytes_per_sector - 2);
                    if (size == 0)
                        size = bytes_per_sector - 2;
                    next_block.track = 0;
                    next_block.sector = size + 1;
                }
                else
                    next_block = AllocBlock(false);
                WriteBlock(block, next_block, data, data_len, offset);
                block = next_block;
                offset += (bytes_per_sector - 2);
            }
        }
        // else TODO: report soft error
        FlushDisk();
    }
}

void EmuD64::StoreFileByName(const char* filename, unsigned char* data, int data_len)
{
    if (GetDirectoryCount()-GetDeletedCount() >= dir_entries_max)
    {
        char exception[80];
        snprintf(exception, sizeof(exception), "directory is full, cannot store file %s", filename);
        show_exception(exception);
    }
    DirStruct* dir = new DirStruct();
    dir->file_type = DirStruct::FileType::PRG;
    int i;
    int filename_len = (int)strlen(filename);
    for (i = 0; i < filename_len && i < DirStruct::dir_name_size; ++i)
        dir->filename[i] = filename[i];
    while (i < DirStruct::dir_name_size)
        dir->filename[i++] = 0xA0; // pad
    StoreFileByStruct(dir, data, data_len);
}

const char* EmuD64::GetDirectoryFormatted()
{
    static char s[dir_entries_max * (DirStruct::dir_name_size + 10)]; // conservative estimate

    strcpy_s(s, sizeof(s), "0 ");
    size_t dest = strlen(s);

    strcat_s(&s[dest], sizeof(s) - dest, "\"");
    ++dest;

    char disk_name[disk_name_size+1];
    DiskName(disk_name, sizeof(disk_name));
    strcat_s(&s[dest], sizeof(s) - dest, disk_name);
    dest += strlen(disk_name);

    strcat_s(&s[dest], sizeof(s) - dest, "\"");
    ++dest;

    char disk_id[disk_id_size+1];
    DiskId(disk_id, sizeof(disk_id));
    strcat_s(&s[dest], sizeof(s) - dest, disk_id);
    dest += strlen(disk_id);

    strcat_s(&s[dest], sizeof(s) - dest, " ");
    ++dest;

    char disk_dos_type[disk_dos_type_size+1];
    DiskDosType(disk_dos_type, sizeof(disk_dos_type));
    strcat_s(&s[dest], sizeof(s) - dest, disk_dos_type);
    dest += strlen(disk_dos_type);

    strcat_s(&s[dest], sizeof(s) - dest, "\n");
    ++dest;

    int count = GetDirectoryCount();
    for (int i = 0; i < count; ++i)
    {
        DirStruct* dir = DirectoryEntry(i);
        if (!((dir->file_type & 7) == DirStruct::FileType::DEL && dir->n_sectors == 0))
        {
            int data_len = 0;
            ReadFileByIndex(i, (unsigned char*)0, data_len);
            const char* s_dir = dir->toString();
            strcat_s(&s[dest], sizeof(s) - dest, s_dir);
            dest += strlen(s_dir);

            strcat_s(&s[dest], sizeof(s) - dest, " ");
            ++dest;

            char s_data_len[6];
            snprintf(s_data_len, sizeof(s_data_len), "%d", data_len);
            strcat_s(&s[dest], sizeof(s) - dest, s_data_len);

            strcat_s(&s[dest], sizeof(s) - dest, "\n");
            ++dest;
        }
    }

    char s_blocks_free[5];
    snprintf(s_blocks_free, sizeof(s_blocks_free), "%d", BlocksFree());
    strcat_s(&s[dest], sizeof(s) - dest, s_blocks_free);

    const char* blocks_free = " BLOCKS FREE.";
    strcat_s(&s[dest], sizeof(s) - dest, blocks_free);
    dest += strlen(blocks_free);

    return s;
}

static void WriteByte(unsigned char * data, int &pos, unsigned char byte)
{
    data[pos++] = byte;
}

static void WriteBytes(unsigned char * data, int &pos, const unsigned char * bytes, int len)
{
    for (int i = 0; i < len; ++i)
        data[pos++] = bytes[i];
}

static void WriteString(unsigned char* data, int& pos, const char* s)
{
    WriteBytes(data, pos, (const unsigned char *)s, (int)strlen(s));
}

static void WriteWord(unsigned char* data, int& pos, unsigned short word)
{
    data[pos++] = word & 0xFF;
    data[pos++] = word >> 8;
}

// construct a Commodore program that represents the directory contents
bool EmuD64::GetDirectoryProgram(unsigned char* data, int& data_size)
{
    char disk_name[disk_name_size+1];
    char disk_id[disk_id_size + 1];
    char dos_type[disk_dos_type_size + 1];
    DiskName(disk_name, sizeof(disk_name));
    DiskId(disk_id, sizeof(disk_id));
    DiskDosType(dos_type, sizeof(dos_type));

    int offset = 0;
    unsigned short addr = 0x801;
    WriteWord(data, offset, addr); // start with load address
    WriteWord(data, offset, addr += 0x1E); // next line pointer
    WriteWord(data, offset, 0); // line number
    WriteByte(data, offset, 18); // RVS
    WriteByte(data, offset, '"');
    for (int i = 0; i < disk_name_size; ++i)
    {
        unsigned char value = disk_name[i];
        if (value == 0xA0)
            value = 0x20;
        WriteByte(data, offset, value);
    }
    WriteByte(data, offset, '"');
    WriteByte(data, offset, ' ');
    WriteBytes(data, offset, (unsigned char*)disk_id, disk_id_size);
    WriteByte(data, offset, ' ');
    WriteBytes(data, offset, (unsigned char*)dos_type, disk_dos_type_size);
    WriteByte(data, offset, 0);

    int count = GetDirectoryCount();
    for (int i = 0; i < count; ++i)
    {
        DirStruct* dir = DirectoryEntry(i);
        if ((dir->file_type & 7) == DirStruct::FileType::PRG)
        {
            int next = offset;
            WriteWord(data, offset, 0); // will patch up next later
            WriteWord(data, offset, dir->n_sectors); // line number
            char s_n_sectors[4];
            int n_s_len = snprintf(s_n_sectors, sizeof(s_n_sectors), "%d", dir->n_sectors);
            for (int j = n_s_len; j <= 3; ++j)
                WriteByte(data, offset, ' ');
            WriteByte(data, offset, '"');
            int j = 0;
            while (j < DirStruct::dir_name_size && dir->filename[j] != 0xA0)
                WriteByte(data, offset, dir->filename[j++]);
            WriteByte(data, offset, '"');
            for (int k = j; k < DirStruct::dir_name_size + 1; ++k)
                WriteByte(data, offset, ' ');
            const char* file_type = dir->FileTypeString();
            WriteString(data, offset, file_type);
            WriteByte(data, offset, 0);
            addr += (offset - next);
            data[next] = (addr & 0xFF);
            data[next + 1] = (addr >> 8);
        }
    }
    int next = offset;
    WriteWord(data, offset, 0); // will patch up next later
    WriteWord(data, offset, BlocksFree()); // line number
    WriteString(data, offset, "BLOCKS FREE.");
    WriteByte(data, offset, 0);
    addr += (offset - next);
    data[next] = (addr & 0xFF);
    data[next + 1] = (addr >> 8);
    WriteWord(data, offset, 0);
    data_size = offset;
    return true;
}

void EmuD64::LoadFromFilenameOrCreate()
{
    if (bytes != 0)
        show_exception("Invalid operation, disk already in memory");

    // check if asked to load .PRG, load .D64 instead, create if doesn't exist
    int filename_len = (int)strlen(filename_d64);
    char* filename = 0;
    if (filename_len > 3 && filename_d64[filename_len - 4] == '.'
        && (filename_d64[filename_len - 3] == 'P' || filename_d64[filename_len - 3] == 'p')
        && (filename_d64[filename_len - 2] == 'R' || filename_d64[filename_len - 2] == 'r')
        && (filename_d64[filename_len - 1] == 'G' || filename_d64[filename_len - 1] == 'g'))
    {
        // make copy for loading later
        filename = new char[filename_len + 1];
        strcpy_s(filename, filename_len + 1, filename_d64);

        // fix disk extension name
        filename_d64[filename_len - 3] = 'd';
        filename_d64[filename_len - 2] = '6';
        filename_d64[filename_len - 1] = '4';
    }

    File fp = SD.open(filename_d64, FILE_READ);
    if (fp && fp.size() == bytes_per_disk)
    {
        bytes = new unsigned char[bytes_per_disk];

        // Arduino/Teensy can only read 64K at a time, so read each track separately
        for (int track = 1; track <= n_tracks; ++track)
        {
            int offset = GetSectorOffset(track, 0);
            int track_bytes = sectors_per_track[track] * bytes_per_sector;
            fp.read(&bytes[offset], track_bytes);
        }
        
        fp.close();
        for (int i = 1; i <= n_tracks; ++i)
            track_dirty[i] = false;

        // if PRG filename is set, assume already exists in D64, so forget filename
        if (filename != 0)
        {
            delete[] filename;
            filename = 0;
        }
    }
    else if (!fp)
    {
        bytes = new unsigned char[bytes_per_disk];        
        InitializeData((unsigned char*)"DISK NAME", (unsigned char*)"ID");
        for (int i = 1; i <= n_tracks; ++i)
            track_dirty[i] = true;
    }
    else
    {
        fp.close();
        char msg[80];
        snprintf(msg, sizeof(msg), "only 35-track disks, no errors supported, expected exactly %d bytes", EmuD64::bytes_per_disk);
        show_exception(msg);
    }

    if (filename != 0)
    {
        File fp = SD.open(filename, FILE_READ);
        if (fp)
        {
            int bin_len = fp.size();
            unsigned char* bin = new unsigned char[bin_len];
            fp.read(bin, bin_len);
            fp.close();
            // upper case only, and remove extension for storing in directory
            filename_len -= 4;
            for (int i = 0; i < filename_len; ++i)
            {
                if (filename[i] >= 'a' && filename[i] <= 'z')
                    filename[i] = filename[i] - 'a' + 'A';
            }
            filename[filename_len] = 0;
            StoreFileByName(filename, bin, bin_len);
            delete[] bin;
        }
        delete[] filename;
    }

    FlushDisk(); // store any changes to disk
}

void EmuD64::FlushDisk()
{
    // perform quick check if any tracks dirty before opening file
    bool dirty = false;
    for (int track = 1; !dirty && track <= n_tracks; ++track)
        if (track_dirty[track])
            dirty = true;

    if (dirty)
    {
        bool new_file = false;
        File fp = SD.open(filename_d64, FILE_WRITE);
        if (fp.size() == 0) // in case couldn't open file, create file
        {
            for (int i = 1; i <= n_tracks; ++i)
                track_dirty[i] = true;
            new_file = true;
        }

        if (fp)
        {
            // do all but directory track first
            for (int track = 1; track <= n_tracks; ++track)
            {
                if (track_dirty[track] && (new_file || track != dir_track))
                {
                    int offset = GetSectorOffset(track, 0);
                    fp.seek(offset);
                    int track_bytes = sectors_per_track[track] * bytes_per_sector;
                    fp.write(&bytes[offset], track_bytes);
                    track_dirty[track] = false;
                }
            }

            // do directory track last
            if (track_dirty[dir_track])
            {
                int offset = GetSectorOffset(dir_track, 0);
                fp.seek(offset);
                int track_bytes = sectors_per_track[dir_track] * bytes_per_sector;
                fp.write(&bytes[offset], track_bytes);
                track_dirty[dir_track] = false;
            }

            fp.close();
        }
    }
}

