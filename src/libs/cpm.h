//! \file cpm.h
//!
//! Handles sectors in the h17disk file format.
//!

#ifndef __CPM_H__
#define __CPM_H__

#include "hi_types.h"

#include <stdio.h>

class H17Disk;

class Sector;


struct DirectoryEntry {
    bool deleted;
    uint8_t status;         /* 0     Serial number        */
    char fullFileName[13];
    char fileName[8];
    char fileExt[3];
    bool readOnly;
    bool systemFile;
    bool archived;

    uint8_t Xh;       // Extent High
    uint8_t Xl;       // Extent Low
    uint16_t Extent;  // Extent
    uint8_t Bc;       // Byte count (doesn't appear to be used in 2.2)
    uint8_t Rc;       // Record count
    uint8_t Al[16];   // Allocation Blocks

    int16_t linkEntry;
};


class CPM
{
public:

    CPM(H17Disk* diskImage);
    ~CPM();

    bool     dumpInfo();

    bool     loadDiskInfo();

    void     printDate(uint16_t date);
    void     fprintDate(uint16_t date);
    void     loadDirectory();
    void     printDirectory(uint16_t clusterNumber);
    void     printDirectoryEntry(uint8_t *entry);
    void     loadDirectoryEntry(uint8_t *entry, uint16_t num);
    void     printEntry(uint16_t entry);

    Sector  *getSector(uint16_t sectorNum);
    void     dumpSector(uint16_t sectorNum);
    void     dumpCluster(uint8_t cluster, uint8_t lastSectorIndex);
    void     dumpFile(uint8_t firstGroup, uint8_t lastGroup, uint8_t lastSectorIndex);
    uint8_t  saveSector(FILE * file, uint16_t sectorNum, uint8_t records);
/*    void     saveCluster(FILE * file,
                         uint8_t cluster,
                         uint8_t lastSectorIndex,
                         uint16_t &sizeInSectors,
                         uint16_t &badSectors);*/
    void     saveBlock(FILE *    file,
                       uint8_t   block,
                       uint8_t   numSectors,
                       uint16_t &sizeInSectors,
                       uint16_t &badSectors);

    uint16_t saveFile(char* filename, uint8_t firstGroup, uint8_t lastGroup, uint8_t lastSectorIndex);

    uint16_t getFreeSpace();

private:

    H17Disk* diskImage_m;
    bool fatalError_m;
    uint8_t  sides_m;
    uint8_t  tracks_m;
    uint8_t  systemTracks_m;
    uint8_t  sectorsPerTrack_m;
    uint16_t bytesPerSector_m;
    //uint8_t  blockSize_m; // number of Sectors in a block
    //uint8_t  directoryBlocks_m; // number of blocks used for directory entries
    uint8_t  numFiles_m;
//    uint16_t  usedSectors_m;

//    DiskInfo diskInfo;
    uint8_t directorySize_m;
    DirectoryEntry directory_m[128];


    FILE *indexFile_m;
};


#endif

