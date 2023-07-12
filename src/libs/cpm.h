//! \file cpm.h
//!
//! Handles sectors in the h17disk file format.
//!

#ifndef __CPM_H__
#define __CPM_H__

#include "hi_types.h"

#include <stdio.h>
#include <map>
#include <vector>
#include <string>

class H17Disk;
class H17DataBlock;

class Sector;

const uint8_t maxUserNum_c = 16;

/*
  Byte | Key | Description
    0   | UU  | User number (0-15)
    1   | F1  | Filename
    2   | F2  | Filename
    3   | F3  | Filename
    4   | F4  | Filename
    5   | F5  | Filename
    6   | F6  | Filename
    7   | F7  | Filename
    8   | F8  | Filename
    9   | T1  | Filetype (hi-bit: read-only)
   10   | T2  | Filetype (hi-bit: system file)
   11   | T3  | Filetype (hi-bit: archive)
   12   | EX  | Extent counter (low byte)
   13   | S1  | Reserved (set to 0)
   14   | S2  | Extent countere (high byte)
   15   | RC  | Number of records
   16   | AL  | Allocation
   17   | AL  |
   18   | AL  |
   19   | AL  |
   20   | AL  |
   21   | AL  |
   22   | AL  |
   23   | AL  |
   24   | AL  |
   25   | AL  |
   26   | AL  |
   27   | AL  |
   28   | AL  |
   29   | AL  |
   30   | AL  |
   31   | AL  |
*/

/*
The translation vectors (XLT 00 through XLTn-l) are located
elsewhere in the BIOS, and simply correspond one-for-one with the
logical sector numbers zero through the sector count-1. The Disk
Parameter Block (DPB) for each drive is more complex. A particular
DPB, which is addressed by one or more DPH's, takes the general form
------------------------------------------------------------
|  SPT  |BSH|BLM|EXM|  DSM  |  DRM  |AL0|AL1|  CKS  |  OFF |
------------------------------------------------------------
   16b   8b  8b  8b    16b     16b   8b  8b    16b     16b

where each is a byte or word value, as shown by the "Sb" or 1I16b"
indicator below the field.

SPT     is the total number of sectors per track

BSH     is the data allocation block shift factor, determined
        by the data block allocation size.

EXM     is the extent mask, determined by the data block
        allocation size and the number of disk blocks.
DSM     determines the total storage capacity of the disk drive
DRM     determines the total number of directory entries which
        can be stored on this drive AL0,AL1 determine reserved
        directory blocks.
CKS     is the size of the directory check vector
OFF     is the number of reserved tracks at the beginning of
        the (logical) disk.
*/

struct DirectoryEntry {
    bool deleted;
    uint8_t status;         /* 0     Serial number        */
    uint8_t userNumber;
    char fullFileName[13];
    char keyFileName[13];
    char fileName[8];
    char fileExt[3];
    bool readOnly;
    bool systemFile;
    bool archived;

    uint8_t Xh;         // Extent High
    uint8_t Xl;         // Extent Low
    uint16_t Extent;    // Extent
    uint8_t Bc;         // Byte count (doesn't appear to be used in 2.2)
    uint8_t Rc;         // Record count
    uint8_t Al[16];     // Allocation Blocks

    int16_t linkEntry;
};

struct FileBlock {

   uint8_t userNum;
   std::string fileName;
   uint16_t nextExpectedExtent;
   uint16_t records;
   std::vector<int> allocBlocks;
};


class CPM
{
public:

    CPM(H17Disk* diskImage);
    ~CPM();

    static bool isValidImage(H17Disk& diskImage);
    static bool validateDirectory(H17DataBlock *diskData, uint8_t sides, uint8_t tracks);
    static bool validateDirectoryEntry(uint8_t *entry);
    static Sector  *getSector(H17DataBlock *diskData, uint8_t sides, uint16_t sectorNum);

    bool     dumpInfo();

    bool     loadDiskInfo();

    void     loadDirectory();
    void     printDirectory(uint16_t clusterNumber);
    void     printDirectoryEntry(uint8_t *entry);
    void     loadDirectoryEntry(uint8_t *entry, uint16_t num);
    void     printEntry(uint16_t entry);

    Sector  *getSector(uint16_t sectorNum);
    void     dumpSector(uint16_t sectorNum,
                        uint8_t numRecords);
    uint8_t  saveSector(FILE     *file,
                        uint16_t  sectorNum,
                        uint8_t   records);
    void     saveBlock(FILE     *file,
                       uint8_t   block,
                       uint16_t  numRecords,
                       uint16_t &sizeInRecords,
                       uint16_t &badSectors);

    bool     listFile(FileBlock fileblock);
    bool     saveFile(FileBlock fileblock);

    bool     listFiles();
    bool     saveAllFiles();

    uint16_t getFreeSpace();
    uint32_t getFreeSpaceInBytes();

private:

    H17Disk *diskImage_m;
    bool     fatalError_m;
    uint8_t  sides_m;
    uint8_t  tracks_m;
    uint8_t  systemTracks_m;
    uint8_t  sectorsPerTrack_m;
    uint16_t bytesPerSector_m;
    uint8_t  blockSize_m;        // number of Sectors in a block
    uint16_t blockSizeInBytes_m;
    uint16_t numBlocks_m;
    uint8_t  firstDataSector_m;
    uint8_t  directoryBlocks_m;  // number of blocks used for directory entries
    uint8_t  numFiles_m;
//    uint16_t  usedSectors_m;

//    DiskInfo diskInfo;
    uint8_t directorySize_m;

    // todo make this dynamic
    DirectoryEntry directory_m[128];

    bool    *freeBlocks_m;
    bool     onlyUserZeroFiles_m;

    std::multimap<std::string, int> directoryMap_m[maxUserNum_c];
    std::map<std::string, FileBlock> fileMap_m[maxUserNum_c];
    FILE *indexFile_m;
};


#endif

