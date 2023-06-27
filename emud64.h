// ////////////////////////////////////////////////////////////////////////////////
// //
// // emud64.h - Class EmuD64
// //   1541 Disk Image Driver - access files, directory from disk image
// //
// // MIT License
// //
// // Copyright (c) 2023 by David R. Van Wagner
// // davevw.com
// //
// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to deal
// // in the Software without restriction, including without limitation the rights
// // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// // copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:
// //
// // The above copyright notice and this permission notice shall be included in all
// // copies or substantial portions of the Software.
// //
// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// // SOFTWARE.
// //
// ////////////////////////////////////////////////////////////////////////////////

// #pragma once

// class EmuD64
// {
// private:
//     unsigned char* bytes;
//     char* filename_d64;
//     bool* track_dirty;

// public:
//     EmuD64(const char* filename_d64); // tested
//     ~EmuD64(); // TODO: TEST

//     class DirStruct;
    
//     void InitializeData(unsigned char* disk_name, unsigned char* id); // TODO: TEST
//     int GetSectorOffset(int track, int sector); // tested
//     void WalkDirectory(bool (*dirFn)(EmuD64* d64, DirStruct* dir, int n, bool last, void*& context), void*& context); // tested
//     int GetDirectoryCount(); // tested
//     int GetDeletedCount();
//     DirStruct* DirectoryEntry(int i); // tested
//     void DiskBAMField(int field_offset, int size, unsigned char* field); // tested
//     void DiskBAMPrintable(int field_offset, int size, char* field); // tested
//     void DiskName(char* disk_name, int size); // tested
//     void DiskId(char* disk_id, int size); // tested
//     void DiskDosType(char* disk_dos_type, int size); // tested
//     int BlocksFree(); // tested
//     void ReadFileByIndex(int i, unsigned char* data, int& length); // tested
//     void ReadFileByName(const char* filename, unsigned char* bytes, int &length); // TODO: TEST
//     void StoreFileByName(const char* filename, unsigned char* data, int data_len); // TODO: TEST
//     const char* GetDirectoryFormatted(); // tested
//     bool GetDirectoryProgram(unsigned char* data, int& data_size); // TODO: TEST

// private:
//     struct BlockStruct
//     {
//         int track;
//         int sector;
//     };

//     void WriteBlock(BlockStruct block, BlockStruct next_block, unsigned char* data, int data_len, int data_offset);
//     BlockStruct AllocBlock(bool directory);
//     void StoreFileByStruct(DirStruct* dir, unsigned char* data, int data_len);
//     bool FindOrAllocDirectoryEntry(DirStruct* dir, int& track, int& sector, int& entry);
//     void LoadFromFilenameOrCreate();
//     void FlushDisk();

// public:
//     static const int sectors_per_disk =
//     (
//         21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 + 21 +
//         19 + 19 + 19 + 19 + 19 + 19 + 19 +
//         18 + 18 + 18 + 18 + 18 + 18 +
//         17 + 17 + 17 + 17 + 17
//     );
//     static const int bytes_per_disk = 256 * sectors_per_disk;
//     static const int dir_track = 18;
//     static const int dir_sector = 1;
//     static const int dir_entry_size = 32;
//     static const int n_tracks = 35;
//     static const int bytes_per_sector = 256;
//     static const int dir_entries_per_sector = bytes_per_sector / dir_entry_size;
//     static const int dir_entries_max = (19/*sectors_per_track[dir_track]*/ - 1) * dir_entries_per_sector;
//     static const int bam_track = dir_track;
//     static const int bam_sector = 0;
//     static const int disk_name_offset = 0x90;
//     static const int disk_name_size = 16;
//     static const int disk_id_offset = 0xA2;
//     static const int disk_id_size = 2;
//     static const int disk_dos_type_offset = 0xA5;
//     static const int disk_dos_type_size = 2;

//     class DirStruct
//     {
//     public:
//         static const int dir_name_size = 16;
//         const int dir_unused_size = 6;
//         enum FileType { DEL = 0, SEQ = 1, PRG = 2, USR = 3, REL = 4 };

//         int next_track;
//         int next_sector;
//         int file_type; // lower 4 bits, 4 unused, 5 for @SAVE, 6 for > Locked, 7 Closed otherwise *
//         int file_track;
//         int file_sector;
//         unsigned char filename[16]; // 16 character, PETSCII, $A0 right padded
//         int rel_track;
//         int rel_sector;
//         int rel_length;
//         unsigned char unused[6]; // 6 bytes unused except GEOS disks
//         unsigned short n_sectors; // 16-bit

//         DirStruct(); // tested

//         void Read(EmuD64* d64, int track, int sector, int n); // tested
//         void ReadData(EmuD64* d64, unsigned char* data, int offset); // tested
//         int Store(EmuD64* d64, int dir_track, int dir_sector, int dir_sector_entry);
//         static char PrintableChar(unsigned char c); // tested
//         void getName(unsigned char* name, int size);
//         const char* FileTypeString(); // tested
//         const char* toString(); // tested
//     }; // class DirStruct 

// }; // class EmuD64
