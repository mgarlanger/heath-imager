//! \file hdos.h
//!
//! Handles sectors in the h17disk file format.
//!

#ifndef __HDOS_H__
#define __HDOS_H__

#include <stdint.h>
#include <stdio.h>

class H17Disk;
class H17DataBlock;
class Sector;

struct DiskInfo {
    uint8_t  volSer;         /* 0     Serial number        */
    uint16_t iDate;          /* 1-2   Date INITed          */
    uint16_t dirSector;      /* 3-4   Start of DIRectory   */
    uint16_t grtSector;      /* 5-6   GRT sector           */ 
    uint8_t  spg;            /* 7     sectors per group    */ //HRJ 2,4,8
    uint8_t  volType;        /* 8     volume type          */ 
    uint8_t  initVer;        /* 9     INIT version used    */ //HRJ 16h or 20h
    uint16_t rgtSector;      /* 10-11 RGT sector           */ 
    uint16_t volSize;        /* 12-13 volume size          */ //HRJ # sectors on disk
    uint16_t sectSize;       /* 14-15 physical sector size */ //HRJ should be 256 dec
    uint8_t  dkFlags;        /* 16    flags                */ //HRJ 0,1,2,3 binary
    uint8_t  label[60];      /* 17-76 disk label           */
    uint16_t reserved;       /* 77-78 reserved             */ 
    uint8_t  spt;            /* 79    sectors per track    */ //HRJ should be 10 dec
    uint8_t  unused[176];    /*       filler               */
};


class HDOS
{
public:

    HDOS(H17Disk* diskImage);
    ~HDOS();

    static bool isValidImage(H17Disk& diskImage);
    //static bool loadDiskInfo(DiskInfo &diskInfo);
    static bool loadDiskInfo(H17DataBlock *diskData, DiskInfo &diskInfo);

    bool     dumpInfo();

    bool     loadDiskInfo();

    void     printDate(uint16_t date);
    void     fprintDate(uint16_t date);
    void     printDirectory(uint16_t clusterNumber);
    bool     loadRGT();
    bool     loadGRT();
    bool     printDirectoryEntry(uint8_t *entry);
    void     printRGT();
    void     printGRT();
    Sector  *getSector(uint16_t sectorNum);
    void     dumpSector(uint16_t sectorNum);
    void     dumpCluster(uint8_t cluster, uint8_t lastSectorIndex);
    void     dumpFile(uint8_t firstGroup, uint8_t lastGroup, uint8_t lastSectorIndex);
    uint8_t  saveSector(FILE * file, uint16_t sectorNum);
    void     saveCluster(FILE * file,
                         uint8_t cluster,
                         uint8_t lastSectorIndex,
                         uint16_t &goodSectors,
                         uint16_t &badSectors);
    uint16_t saveFile(char* filename, uint8_t firstGroup, uint8_t lastGroup, uint8_t lastSectorIndex);

    uint16_t getFreeSpace();

private:

    H17Disk*      diskImage_m;
    H17DataBlock* diskData_m;

    uint8_t   sides_m;
    uint8_t   tracks_m;
    uint8_t   numFiles_m;
    uint16_t  usedSectors_m;

    DiskInfo diskInfo_m;
    uint8_t  RGT[256];
    uint8_t  GRT[256];
    uint16_t numClusters_m;

    bool fatalError_m;

    FILE *indexFile_m;
};


#endif
