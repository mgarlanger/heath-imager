//! \file hdos.cpp
//!
//! Handles HDOS disk images.
//!

#include "hdos.h"
#include "h17disk.h"
#include "h17block.h"

#include "sector.h"
#include <stdio.h>

#define SUMMARY 1

//!  Constructor
//!
//!  Defaults to single-sided, 40 tracks
//!
HDOS::HDOS(H17Disk* diskImage): diskImage_m(diskImage)
{
    diskData_m = (H17DataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);
    H17DiskFormatBlock *diskFormat = (H17DiskFormatBlock *)
                diskImage_m->getH17Block(H17Block::DiskFormatBlock_c);

    sides_m = diskFormat->getSides();
    tracks_m = diskFormat->getTracks();
    fatalError_m = false;
    // indexFile_m = fopen("0_index.info","w");

    if (!loadDiskInfo())
    {
        fprintf(indexFile_m, "Unable to load disk info\n");
        fatalError_m = true;
    }

    // early INIT versions (1.5,1.6) did not set volSize, so default to 400
    // TODO make sure INIT version is expected.
    if (diskInfo_m.volSize == 0)
    {
        diskInfo_m.volSize = 400;
    }
    if (diskInfo_m.volSize != 400 &&
        diskInfo_m.volSize != 800 &&
        diskInfo_m.volSize != 1600)
    {
        fatalError_m = true;
    }

    numClusters_m = diskInfo_m.volSize / diskInfo_m.spg;
    if (!loadRGT())
    {
        fprintf(indexFile_m, "Unable to load RGT\n");
        fatalError_m = true;
    }
    if (!loadGRT())
    {
        fprintf(indexFile_m, "Unable to load GRT\n");
        fatalError_m = true;
    }

    numFiles_m = 0;
    usedSectors_m = 0;

    if (!fatalError_m)
    {
       indexFile_m = fopen("0_index.info","w");
    }
}

HDOS::~HDOS()
{
   fclose(indexFile_m);
}

bool
HDOS::loadDiskInfo(H17DataBlock *diskData, DiskInfo &diskInfo)
{

    Sector *sector = diskData->getSector(9);

    uint8_t error = sector->getErrorCode();
    bool earlyVer = false;

    if (error != 0)
    {
        printf("Error on label sector, aborting: %d\n", error);

        return false;
    }

    uint8_t *data = sector->getSectorData();

    // Disk Volume number
    diskInfo.volSer     = data[0];
    // Init date
    diskInfo.iDate      = data[2] << 8 | data[1];

    // Directory Sector
    diskInfo.dirSector  = data[4] << 8 | data[3];
    // GRT Sector
    diskInfo.grtSector  = data[6] << 8 | data[5];
    // Sectors per Group
    diskInfo.spg        = data[7];
    if ((diskInfo.spg != 2) && (diskInfo.spg != 4) &&
        (diskInfo.spg != 8))
    {
        printf("Wrong spg: %d\n", diskInfo.spg);
        return false;
    }
    // Volume Type
    diskInfo.volType    = data[8];

    // Init version
    diskInfo.initVer    = data[9];
    if ((diskInfo.initVer == 0x10) || (diskInfo.initVer == 0x15) ||
        (diskInfo.initVer == 0x16) ||
        // had one disk with DG
        (diskInfo.initVer == 0x00))
    {
        earlyVer = true;
    }
    else if ((diskInfo.initVer != 0x20) && (diskInfo.initVer != 0x30))
    {
        printf("Unknown version: %02x\n", diskInfo.initVer);
        return false;
    }

    // RGT Sector
    diskInfo.rgtSector  = data[11] << 8 | data[10];

    // Volume Size (Number of sectors)
    diskInfo.volSize    = data[13] << 8 | data[12];

    if (earlyVer && diskInfo.volSize == 0)
    {
        diskInfo.volSize = 400;
    }
    if ((diskInfo.volSize != 400) && (diskInfo.volSize != 800) &&
        (diskInfo.volSize != 1600))
    {
        printf("Wrong volSize: %d\n", diskInfo.volSize);
       return false;
    }

    if ((diskInfo.dirSector >= diskInfo.volSize) &&
        (diskInfo.grtSector >= diskInfo.volSize) &&
        (diskInfo.rgtSector >= diskInfo.volSize))
    {
        printf("Sector out of range:\n  dirSector: %d\n  grtSector: %d\n  rgtSector: %d\n",
              diskInfo.dirSector,
              diskInfo.grtSector,
              diskInfo.rgtSector
        );
        return false;
    }

    // Sector Size (256) for hard-sectors disks
    diskInfo.sectSize   = data[15] << 8 | data[14];

    if (earlyVer && (diskInfo.sectSize == 0 || diskInfo.sectSize == 255))
    {
        diskInfo.sectSize = 256;
    }

    // validate sector size
    if (diskInfo.sectSize != 256)
    {
        printf("Wrong sectSize: %d\n", diskInfo.sectSize);
        return false;
    }

    // Disk Flags
    // Bit 0 Number of Sides
    //     0 - 1 side
    //     1 - 2 sides
    // Bit 1 Tracks
    //     0 - 40 Tracks
    //     1 - 80 Tracks
    diskInfo.dkFlags    = data[16];

    // check for set unused bit
    if ((diskInfo.dkFlags & 0xfc) != 0) {
        printf("Wrong dkFlags: %d\n", diskInfo.dkFlags);
        return false;
    }

    printf("Disk format: %cS %c0T\n",
           diskInfo.dkFlags & 0x01 ? 'D' : 'S',
           diskInfo.dkFlags & 0x02 ? '8' : '4');

    // Disk Label
    for (int i = 0; i < 60; i++)
    {
        diskInfo.label[i] = data[17+i];
    }
    diskInfo.reserved   = data[78] << 8 | data[77];

    printf("reserved: %d", diskInfo.reserved);

    // Sectors per Track (10 for Hard-sectored)
    diskInfo.spt        = data[79];
    printf("Sectors per Track: %d", diskInfo.spt);

    // TODO determine if spt is set on even version 1.0 disks, if
    // so, then fail if it's not 10.

    for(int i = 0; i < 176; i++)
    {
        diskInfo.unused[i] = data[i + 80];
    }

    return true;
}


bool
HDOS::isValidImage(H17Disk& diskImage)
{
    H17DataBlock   *diskData = (H17DataBlock *) diskImage.getH17Block(H17Block::DataBlock_c);
    // H17DiskFormatBlock *diskFormat = (H17DiskFormatBlock *)
    //             diskImage.getH17Block(H17Block::DiskFormatBlock_c);

    // uint8_t sides = diskFormat->getSides();
    // uint8_t tracks = diskFormat->getTracks();

    DiskInfo diskInfo;


    if (!HDOS::loadDiskInfo(diskData, diskInfo))
    {
        return false;
    }


    // TODO add validation for the special sectors, directory structure, etc.


    return true;
}

bool
HDOS::dumpInfo()
{

    if (fatalError_m)
    {
        printf("Fatal Error\n");
        return false;
    }

#if SUMMARY

    fprintf(indexFile_m, "Disk info\n");
    fprintf(indexFile_m, "   Serial Number: %u\n", diskInfo_m.volSer);

    if (diskInfo_m.dkFlags & 0x01)
    {
        fprintf(indexFile_m, "   Double-Sided\n");
        if (sides_m != 2)
        {
             printf("    ERROR - imaged as single-sided\n");
             fprintf(indexFile_m, "    ERROR - imaged as single-sided\n");
        }
    }
    else
    {
        fprintf(indexFile_m, "   Single-Sided\n");
        if (sides_m != 1)
        {
             printf("    ERROR - imaged as double-sided\n");
             fprintf(indexFile_m, "    ERROR - imaged as double-sided\n");
        }
    }
    if (diskInfo_m.dkFlags & 0x02)
    {
        fprintf(indexFile_m, "   80-Track\n");
        if (tracks_m != 80)
        {
             printf("    ERROR - imaged at %d tracks\n", tracks_m);
             fprintf(indexFile_m, "    ERROR - imaged at %d tracks\n", tracks_m);
        }
    }
    else
    {
        fprintf(indexFile_m, "   40-Track\n");
        if (tracks_m != 40)
        {
             printf("    ERROR - imaged at %d tracks\n", tracks_m);
             fprintf(indexFile_m, "    ERROR - imaged at %d tracks\n", tracks_m);
        }
    }
    fprintf(indexFile_m, "\n    Label:        '");
    for (int i = 0; i < 60; i++)
    {
        fprintf(indexFile_m, "%c", diskInfo_m.label[i]);
    }
    fprintf(indexFile_m, "'\n\n");
    fprintf(indexFile_m, "Name    .Ext    Size      Date          Flags\n");
#else

    printf("Disk Format Info:\n");
    printf("   Sides: %d\n", sides_m);
    printf("   Tracks: %d\n", tracks_m);

    printf(" Sector 9 Info\n");
    printf("    Serial Number: %u\n", diskInfo_m.volSer);

    printf("    Init Date:     ");
    printDate(diskInfo_m.iDate);
    printf("\n");

    printf("    Start of DIR:  %d\n", diskInfo_m.dirSector);
    printf("    GRT Sector:    %d\n", diskInfo_m.grtSector);
    printf("    Sect Per Grp:  %d\n", diskInfo_m.spg);
    printf("    Volume Type:   %d\n", diskInfo_m.volType);
    printf("    Init Vers:     %d.%d\n", (diskInfo_m.initVer >> 4), diskInfo_m.initVer & 0xf);
    printf("    RGT Sector:    %d\n", diskInfo_m.rgtSector);
    printf("    Volume Size:   %d\n", diskInfo_m.volSize);
    printf("    Sector Size:   %d\n", diskInfo_m.sectSize);
    printf("    DK Flags:      %d\n", diskInfo_m.dkFlags);
    printf("    Label:        '");
    for (int i = 0; i < 60; i++)
    {
        printf("%c", diskInfo_m.label[i]);
    }
    printf("'\n");
    printf("    Reserved:      %d\n", diskInfo_m.reserved);
    printf("    Sector/Track:  %d\n", diskInfo_m.spt);
    /*
    for(int i = 0; i < 176; i++)
    {
        printf("    unused[%02d]:    %d (%c)\n", i, diskInfo_m.unused[i], diskInfo_m.unused[i]);
    }
    */
    printf("   numClusters_m: %d\n", numClusters_m);
    printRGT();
    printGRT();
    printf("Name    .Ext    Size      Date          Flags\n");
#endif


    printDirectory(diskInfo_m.dirSector);
    //   25 Files, Using 354 Sectors (22 Free)
    uint16_t freeSpace = getFreeSpace();

    fprintf(indexFile_m, "\n   %d Files, Using %d Sectors (%d Free)\n\n",
            numFiles_m,
            usedSectors_m,
            freeSpace);

    return true;
}


// returns number of free sectors
uint16_t
HDOS::getFreeSpace()
{
    uint16_t space = 0;

    // Free chain starts with 0
    uint8_t cluster = GRT[0];

    while(cluster)
    {
        space += diskInfo_m.spg;

        cluster = GRT[cluster];
    }

    return space;
}


bool
HDOS::loadDiskInfo()
{

    diskData_m = (H17DataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);

    Sector *sector = diskData_m->getSector(9);

    uint8_t error = sector->getErrorCode();

    if (error != 0)
    {
        printf("Error on label sector, aborting: %d\n", error);

        return false;
    }

    uint8_t *data = sector->getSectorData();

    diskInfo_m.volSer = data[0];
    diskInfo_m.iDate  = data[2] << 8 | data[1];
    diskInfo_m.dirSector  = data[4] << 8 | data[3];
    diskInfo_m.grtSector  = data[6] << 8 | data[5];
    diskInfo_m.spg = data[7];
    diskInfo_m.volType = data[8];
    diskInfo_m.initVer = data[9];
    diskInfo_m.rgtSector = data[11] << 8 | data[10];
    diskInfo_m.volSize   = data[13] << 8 | data[12];
    diskInfo_m.sectSize  = data[15] << 8 | data[14];
    diskInfo_m.dkFlags   = data[16];
    for (int i = 0; i < 60; i++)
    {
        diskInfo_m.label[i] = data[17+i];
    }
    diskInfo_m.reserved = data[78] << 8 | data[77];
    diskInfo_m.spt   = data[79];

    for(int i = 0; i < 176; i++)
    {
        diskInfo_m.unused[i] = data[i + 80];
    }

    return true;
}

void
HDOS::fprintDate(uint16_t date)
{
    // zero means "No-Date"
    if (date == 0) {
        fprintf(indexFile_m, " No-Date ");
        return;
    }

    static char monthNames[][4] = {
        "Jan", "Feb", "Mar",
        "Apr", "May", "Jun",
        "Jul", "Aug", "Sep",
        "Oct", "Nov", "Dec"
    };

    int day  = date & 0x1f;
    int mon  = (date >> 5) & 0xf;
    int year = ((date >> 9) & 0x7f) + 70;

    fprintf(indexFile_m, "%02d-%s-%02d", day, monthNames[mon-1], year);
}

void
HDOS::printDate(uint16_t date)
{
    // TODO determine when to print "No-Date"
    //
    int day = date & 0x1f;
    int mon = (date >> 5) & 0xf;
    int year = ((date >> 9) & 0x7f) + 70;

    printf("%02d/%02d/%02d", mon, day, year);
}

Sector *
HDOS::getSector(uint16_t sectorNum)
{
    diskData_m = (H17DataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);

    // if single-sided, direct access is available
    if (sides_m == 1)
    {
        return diskData_m->getSector(sectorNum);
    }
    else
    {
        int sectorNo = sectorNum % 10;
        int sideNum = (sectorNum / 10) & 0x01;
        int trackNum = (sectorNum / 20);

        return diskData_m->getSector(sideNum, trackNum, sectorNo);
    }
}


void
HDOS::printDirectory(uint16_t clusterNumber)
{
    uint8_t cluster[512];

    Sector *sector = getSector(clusterNumber);;
    Sector *sector2 = getSector(clusterNumber + 1);;

    uint8_t *data = sector->getSectorData();
    uint8_t *data2 = sector2->getSectorData();

    for(int i = 0; i < 256; i++)
    {
        cluster[i] = data[i];
        cluster[i + 256] = data2[i];
    }

#if SUMMARY
    bool endOfDirectory = false;

    for (int f = 0; f < 22; f++)
    {
        if (!endOfDirectory)
        {
            endOfDirectory = printDirectoryEntry(&cluster[f * 23]);
        }
    }

    int pos = 22*23;

    if (cluster[pos++] != 0)
    {
        fprintf(indexFile_m, "Unexpected non-zero byte in directry cluster\n");
    }

    // skipping entry length
    pos++;

    // TODO determine if this should be checked
    //  int blockNumber = cluster[pos+1] << 8 | cluster[pos];
    pos += 2;

    int nextBlockNumber = cluster[pos+1] << 8 | cluster[pos];

    pos += 2;

    if (!endOfDirectory && nextBlockNumber)
    {
        printDirectory(nextBlockNumber);
    }

#else
    printf("Directory Cluster:\n");

    for (int f = 0; f < 22; f++)
    {
        printDirectoryEntry(&cluster[f * 23]);
        printf("-----------------------------------------\n");
    }

    printf(" -- end of directory entries\n");
    int pos = 22*23;
    if (cluster[pos++] != 0)
    {
        printf("Unexpected non-zero byte in directry cluster\n");
    }

    printf("   Entry Length: %d\n", cluster[pos++]);
    int blockNumber = cluster[pos+1] << 8 | cluster[pos];
    pos += 2;
    int nextBlockNumber = cluster[pos+1] << 8 | cluster[pos];
    pos += 2;
    printf("   This cluster block number: %d\n", blockNumber);
    printf("   Next cluster block number: %d\n", nextBlockNumber);
    if (nextBlockNumber)
    {
        printDirectory(nextBlockNumber);
    }
    else
    {
        printf("----End of Directories---\n");
    }
#endif

}

bool
HDOS::printDirectoryEntry(uint8_t *entry)
{
    bool retVal = false;

    int pos = 0;

    int firstGroup;
    int lastGroup;
    int sectorIndex;
    bool deleted = false;
    char fileName[13];
    int fileNamePos = 0;

#if SUMMARY

    if (entry[pos] == 254)
    {
        deleted = true;
        retVal = true;
    }
    else if (entry[pos] == 255)
    {
        deleted = true;
    }

    if (deleted) {
        return retVal;
    }

    numFiles_m++;

    for (int i = 0; i < 8; i++)
    {
        if (entry[pos])
        {
            fileName[fileNamePos++] = entry[pos];
            fprintf(indexFile_m, "%c", entry[pos]);
        }
        else
        {
            fprintf(indexFile_m, " ");
        }
        pos++;
    }
    fprintf(indexFile_m, ".");
    fileName[fileNamePos++] = '.';
    for (int j = 0; j < 3; j++)
    {
        if (entry[pos])
        {
            fileName[fileNamePos++] = entry[pos];
            fprintf(indexFile_m, "%c", entry[pos]);
        }
        else
        {
            fprintf(indexFile_m, " ");
        }
        pos++;
    }
    fileName[fileNamePos] = 0;

    fprintf(indexFile_m, "    ");

    // skip project
    pos++;
    // skip version
    pos++;
    // skip cluster factor
    pos++;

    // TODO display flags
    uint8_t flags = entry[pos++];
    //fprintf(indexFile_m, "Flags: %d\n", flags);
    // pos++;

    // skip reserved
    pos++;
    firstGroup  = entry[pos++];
    lastGroup   = entry[pos++];
    sectorIndex = entry[pos++];

    // TODO consider setting file date
    // uint16_t creationDate = entry[pos+1] << 8 | entry[pos];
    pos += 2;
    // TODO consider setting file date
    uint16_t modificationDate = entry[pos+1] << 8 | entry[pos];
    pos += 2;

    uint16_t errorSectors = saveFile(fileName, firstGroup, lastGroup, sectorIndex);

    fprintDate(modificationDate);
    fprintf(indexFile_m, "       ");

    if (flags & 0x80) {
        fprintf(indexFile_m, "S");
    }
    if (flags & 0x40) {
        fprintf(indexFile_m, "L");
    }
    if (flags & 0x20) {
        fprintf(indexFile_m, "W");
    }
    if (flags & 0x10) {
        fprintf(indexFile_m, "C");
    }
    fprintf(indexFile_m, "\n");
    if (errorSectors)
    {
        fprintf(indexFile_m, "ERROR - %d bad sectors in previous file\n", errorSectors);
    }

#else
    if (entry[pos] == 254)
    {
        deleted = true;
        printf("<DELETED-254>: %d\n", entry[pos+1]);
        if (entry[pos + 1] == 0)
        {
            return retVal;
        }
    }
    if (entry[pos] == 255)
    {
        deleted = true;
        printf("<DELETED-255>: %d\n", entry[pos+1]);
    }
    // printf("     fileStatus: %d\n", entry[pos]);
    printf("     fileName: '");
    for (int i = 0; i< 8; i++)
    {
        if (entry[pos])
        {
            fileName[fileNamePos++] = entry[pos];
            printf("%c", entry[pos]);
        }
        else
        {
            printf(" ");
        }
        pos++;
    }
    printf(".");
    fileName[fileNamePos++] = '.';
    for (int j = 0; j < 3; j++)
    {
        if (entry[pos])
        {
            fileName[fileNamePos++] = entry[pos];
        }
        printf("%c", entry[pos++]);
    }

    printf("'\n");
    fileName[fileNamePos++] = 0;
    printf("     fileName: '");
    for (int i = 0; i< 8; i++)
    {
        printf(" %02x", entry[pos - 11 + i]);
    }
    printf(".");
    for (int j = 0; j < 3; j++)
    {
        printf(" %02x", entry[pos - 3 + j]);
    }
    printf("'\n");

    printf("     project:  %d\n", entry[pos++]);
    printf("     version:  %d\n", entry[pos++]);
    printf("     cluster factor: %d\n", entry[pos++]);
    printf("     flags:    %d\n", entry[pos++]);
    printf("     reserved: %d\n", entry[pos++]);
    firstGroup = entry[pos++];
    lastGroup = entry[pos++];
    sectorIndex = entry[pos++];
    printf("     first group #: %d\n", firstGroup);
    printf("     last group #:  %d\n", lastGroup);
    printf("     last sector idx:  %d\n", sectorIndex);

    uint16_t creationDate = entry[pos+1] << 8 | entry[pos];
    pos += 2;
    uint16_t modificationDate = entry[pos+1] << 8 | entry[pos];
    pos += 2;
    printf("     Creation data: ");
    printDate(creationDate);
    printf("\n");
    printf("     Modification data: ");
    printDate(modificationDate);
    printf("\n");
    if (!deleted)
    {
        printf("Save file:\n");

        saveFile(fileName, firstGroup, lastGroup, sectorIndex);
    }
    printf("-----------------------------\n");

#endif

    return retVal;
}

void
HDOS::dumpSector(uint16_t sectorNum)
{
    Sector  *sector = getSector(sectorNum);
    uint8_t *data   = sector->getSectorData();

    for(int i = 0; i < 256; i++)
    {
        printf("%c", data[i]);
    }
}

void
HDOS::dumpCluster(uint8_t cluster, uint8_t lastSectorIndex)
{
    uint16_t sectorNum = cluster * diskInfo_m.spg;

    for (int i = 0; i < lastSectorIndex; i++)
    {
        dumpSector(sectorNum + i);
    }

}

uint8_t
HDOS::saveSector(FILE     *file,
                 uint16_t  sectorNum)
{
    Sector  *sector = getSector(sectorNum);

    if (!sector) {
        printf("Sector not found\n");
        return 0;
    }
    uint8_t *data   = sector->getSectorData();
    if (!data) {
        printf("Data not found\n");
        return 0;
    }

    uint8_t  error  = sector->getErrorCode();

    fwrite(data, 256, 1, file);

    return error;
}

void
HDOS::saveCluster(FILE     *file,
                  uint8_t   cluster,
                  uint8_t   lastSectorIndex,
                  uint16_t &sizeInSectors,
                  uint16_t &badSectors)
{
    uint16_t sectorNum = cluster * diskInfo_m.spg;

    for (int i = 0; i < lastSectorIndex; i++)
    {
        usedSectors_m++;
        sizeInSectors++;
        if (saveSector(file, sectorNum + i) > 0)
        {
            badSectors++;
        }
    }
}

void
HDOS::dumpFile(uint8_t firstGroup,
               uint8_t lastGroup,
               uint8_t lastSectorIndex)
{
    int pos   = firstGroup;
    int count = 0;

    printf("file data:\n----------------------\n");
    while ((pos != lastGroup) && (count++ < numClusters_m))
    {
        dumpCluster(pos, diskInfo_m.spg);
        pos = GRT[pos];
    }
    dumpCluster(pos, lastSectorIndex);
    printf("--------------------\nEnd file data\n");
    if (count > numClusters_m)
    {
        printf("\n-------------\n  Bad Chain - too many links\n------------\n");
    }
    else if (GRT[pos] != 0)
    {
        printf("\n-------------\n  Bad Chain - last cluster not zero: %d\n------------\n", GRT[pos]);
    }
}

uint16_t
HDOS::saveFile(char    *filename,
               uint8_t  firstGroup,
               uint8_t  lastGroup,
               uint8_t  lastSectorIndex)
{
    FILE     *file          = fopen(filename, "w");
    int       pos           = firstGroup;
    int       count         = 0;
    uint16_t  sizeInSectors = 0;
    uint16_t  badSectors    = 0;

#if SUMMARY
    while ((pos != lastGroup) && (count++ < numClusters_m))
    {
        saveCluster(file, pos, diskInfo_m.spg, sizeInSectors, badSectors);
        pos = GRT[pos];
    }
    saveCluster(file, pos, lastSectorIndex, sizeInSectors, badSectors);

    //if (badSectors)
    //{
    //    fprintf(indexFile_m, "   Bad sector count: %d\n", badSectors);
    //}
    // fprintf(indexFile_m, "   Sector count: %d\n", goodSectors + badSectors);
    fprintf(indexFile_m, "%-8d", sizeInSectors);

    if (count > numClusters_m)
    {
        fprintf(indexFile_m, "Bad Chain - too many links | ");
    }
    else if (GRT[pos] != 0)
    {
        fprintf(indexFile_m, "Bad Chain - last cluster not zero: %d | ", GRT[pos]);
    }

#else
    printf("file data:\n----------------------\n");
    while ((pos != lastGroup) && (count++ < numClusters_m))
    {
        //saveCluster(file, pos, diskInfo_m.spg);
        pos = GRT[pos];
    }
    //saveCluster(file, pos, lastSectorIndex);
    printf("--------------------\nEnd file data\n");
    if (count > numClusters_m)
    {
        printf("\n-----------\n  Bad Chain - too many links\n-----------\n");
    }
    else if (GRT[pos] != 0)
    {
        printf("\n-----------\n  Bad Chain - last cluster not zero: %d\n-----------\n", GRT[pos]);
    }
#endif

    return badSectors;
}


bool
HDOS::loadRGT()
{
   Sector  *sector = getSector(diskInfo_m.rgtSector);
   uint8_t  error  = sector->getErrorCode();

   if (error != 0)
   {
       printf("Error reading RGT sector: %d\n", error);
       return false;
   }

   uint8_t * data = sector->getSectorData();

   for (int i = 0; i < numClusters_m; i++)
   {
      RGT[i] = data[i];
   }

   return true;
}

void
HDOS::printRGT()
{
   for (int i = 0; i < numClusters_m; i++)
   {
        printf("RGT[%d]: %s\n", i, RGT[i] == 1 ? "Valid" : "Blocked");
   }
}

bool
HDOS::loadGRT()
{
   Sector *sector = getSector(diskInfo_m.grtSector);
   uint8_t error = sector->getErrorCode();

   if (error != 0)
   {
       printf("Error reading GRT sector: %d\n", error);
       return false;
   }

   uint8_t * data = sector->getSectorData();

   for (int i = 0; i < numClusters_m; i++)
   {
       GRT[i] = data[i];
   }

   return true;
}

void
HDOS::printGRT()
{
   for (int i = 0; i < numClusters_m; i++)
   {
        printf("GRT[%d]: %d\n", i, GRT[i]);
   }
}

