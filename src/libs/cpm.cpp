//! \file cpm.cpp
//!
//! Handles CP/M disk images.
//!

#include "cpm.h"
#include "h17disk.h"
#include "h17block.h"

#include "sector.h"
#include <stdio.h>

#define CPMDEF 1

uint8_t h17DiskSkew[10] = { 0, 4, 8, 2, 6, 1, 5, 9, 3, 7 };

//!  Constructor
//!
//!  Defaults to single-sided, 40 tracks
//!
CPM::CPM(H17Disk* diskImage): diskImage_m(diskImage)
{
   H17DiskFormatBlock *diskFormat = (H17DiskFormatBlock *) 
                diskImage_m->getH17Block(H17Block::DiskFormatBlock_c);

   // basically constants  
   systemTracks_m = 3;
   sectorsPerTrack_m = 10;
   bytesPerSector_m = 256;
   fatalError_m = false;
 
   sides_m = diskFormat->getSides();
   tracks_m = diskFormat->getTracks();
/*
   if (sides_m == 1)
   {
      if (tracks_m == 40)
      {
          // 100k
          blockSize_m = 4;
          directoryBlocks_m = 2;
      }
      else
      {
          // 200k 
          // 80 tracks
          // TODO determine this - not sure what the right parameters are
          blockSize_m = 4; 
          directoryBlocks_m = 2;
      }
   }
   else 
   {
      // double-sided
      if (tracks_m == 40)
      {
          // 200k
          // TODO determine this - not sure what the right parameters are
          blockSize_m = 4;
          directoryBlocks_m = 2;
      }
      else
      {
          // 400k 
          // 80 tracks
          // TODO verify this - this seems right
          blockSize_m = 8; 
          directoryBlocks_m = 2;
      }
   }
*/
   //fatalError_m = false;
   indexFile_m = fopen("0_index.info","w");

   if (!loadDiskInfo())
   {
       printf("Unable to load disk info\n");
       fatalError_m = true;
   }

   numFiles_m = 0;
   //usedSectors_m = 0;
}

CPM::~CPM()
{
   fclose(indexFile_m);
}



bool
CPM::dumpInfo()
{
   if (fatalError_m)
   {
       printf("Fatal Error\n");
       return false;
   }

   printf("Sides: %d\n", sides_m);
   printf("Tracks: %d\n", tracks_m);

   return true;
}

uint16_t
CPM::getFreeSpace()
{

   uint16_t space = 0;
#if CPMDEF
#else

   uint8_t cluster = GRT[0];

   while(cluster)
   {
      space += diskInfo.spg;

      cluster = GRT[cluster];
   }
#endif

   return space;
}


bool
CPM::loadDiskInfo()
{

   //H17DiskDataBlock *diskData = (H17DiskDataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);
   loadDirectory();
/*
   for(int dirSector = 30; dirSector < 38; dirSector++)
   {
       Sector *sector = getSector(dirSector);

       uint8_t *data = sector->getSectorData();

       for (uint8_t entry = 0; entry < 8; entry++)
       {
           printDirectoryEntry(&data[entry*32]);
       }
   }
*/

   return true;
}

void
CPM::fprintDate(uint16_t date)
{
#if CPMDEF
#else
    static char monthNames[][4] = {
       "Jan", "Feb", "Mar",
       "Apr", "May", "Jun",
       "Jul", "Aug", "Sep",
       "Oct", "Nov", "Dec" 
    };

    int day = date & 0x1f;
    int mon = (date >> 5) & 0xf;
    int year = ((date >> 9) & 0x7f) + 70;

    fprintf(indexFile_m, "%02d-%s-%02d", day, monthNames[mon-1], year);
#endif

}

void
CPM::printDate(uint16_t date)
{
#if CPMDEF
#else
    int day = date & 0x1f;
    int mon = (date >> 5) & 0xf;
    int year = ((date >> 9) & 0x7f) + 70;

    printf("%02d/%02d/%02d", mon, day, year);
#endif

}

Sector  *
CPM::getSector(uint16_t sectorNum) {
    H17DiskDataBlock *diskData = (H17DiskDataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);

    // if single-sided, direct access is available
    if (sides_m == 1)
    {
        int sectorNo = sectorNum % 10;
        int physicalSector = h17DiskSkew[sectorNo];
        int sideNum = 0;
        int trackNum = (sectorNum / 10);
        return diskData->getSector(sideNum, trackNum, physicalSector);
        //return diskData->getSector(trackNum * 10 + physicalSector);
    }
    else
    {
        int sectorNo = sectorNum % 10;
        int physicalSector = h17DiskSkew[sectorNo];
        int sideNum = (sectorNum / 10) & 0x01;
        int trackNum = (sectorNum / 20);

        return diskData->getSector(sideNum, trackNum, physicalSector);
    }
}

void
CPM::loadDirectory()
{
   uint16_t directoryEntry = 0;

   for(int dirSector = 30; dirSector < 38; dirSector++)
   {   
       Sector *sector = getSector(dirSector);
       
       uint8_t *data = sector->getSectorData();
       
       for (uint8_t entry = 0; entry < 8; entry++)
       {   
           loadDirectoryEntry(&data[entry*32], directoryEntry++);
       }
   }

   directorySize_m = directoryEntry;

   for(uint16_t i = 0; i < directorySize_m; i++)
   {
       printEntry(i); 
   }
}

void
CPM::printEntry(uint16_t num)
{
    printf("Entry #%d:\n", num);
    if (directory_m[num].deleted)
    {
        printf("<deleted>\n");
        return;
    }
    printf("Name: ");
    for(int i = 0; i < 8; i++)
    {
       printf("%c", directory_m[num].fileName[i]);
    }
    printf(".");
    for(int i = 0; i < 3; i++)
    {
       printf("%c", directory_m[num].fileExt[i]);
    }
    printf("\n");
    printf("FullName: %s\n", directory_m[num].fullFileName);

    printf("Xl: %d\n", directory_m[num].Xl);
    printf("Xh: %d\n", directory_m[num].Xh);
    printf("Extent: %d\n", directory_m[num].Extent);
    printf("Bc: %d\n", directory_m[num].Bc);
    printf("Rc: %d\n", directory_m[num].Rc);
    for(int i = 0; i < 16; i++)
    {
       printf("Al: %d\n", directory_m[num].Al[i]);
    }
}

void
CPM::printDirectory(uint16_t clusterNumber)
{

   for(int dirSector = 30; dirSector < 38; dirSector++)
   {
       Sector *sector = getSector(dirSector);

       uint8_t *data = sector->getSectorData();

       for (uint8_t entry = 0; entry < 8; entry++)
       {
           printDirectoryEntry(&data[entry*32]);
       }
   }

}

void
CPM::loadDirectoryEntry(uint8_t *entry, uint16_t entryNum)
{
   int pos = 0;

   // printf("--------------------\n");
   // printf("Status: 0x%02x", entry[pos]);
   if (entry[pos++] == 0xE5)
   {
      directory_m[entryNum].deleted = true;
      return;
   }
   directory_m[entryNum].deleted = false;

   for (int i = 0; i < 8; i++)
   {
      if (entry[pos + i] & 0x80)
      {
         printf("unexpected high bit set on filename pos %d\n", i);
      }
   }
   directory_m[entryNum].readOnly = (entry[pos + 8] & 0x80);
   directory_m[entryNum].systemFile = (entry[pos + 9] & 0x80);
   directory_m[entryNum].archived = (entry[pos + 10] & 0x80);

   uint8_t fullnamePos = 0;

   for (int i = 0; i < 8; i++)
   {
      char ch = entry[pos++];

      directory_m[entryNum].fileName[i] = ch;
      if(ch != 32)
      {
          directory_m[entryNum].fullFileName[fullnamePos++] = ch;
      }
   }

   directory_m[entryNum].fullFileName[fullnamePos++] = '.';
   
   for (int i = 0; i < 3; i++)
   {
      char ch = entry[pos++];

      directory_m[entryNum].fileExt[i] = ch;
      if(ch != 32)
      {
          directory_m[entryNum].fullFileName[fullnamePos++] = ch;
      }
   }
   directory_m[entryNum].fullFileName[fullnamePos] = 0;

   uint8_t Xl = entry[pos++];
   directory_m[entryNum].Xl = Xl;
   directory_m[entryNum].Bc = entry[pos++];
   uint8_t Xh = entry[pos++];
   directory_m[entryNum].Xh = Xh;
   directory_m[entryNum].Extent = (Xh << 5) | (Xl & 0x1f);
   directory_m[entryNum].Rc = entry[pos++];

   for (int i = 0; i < 16; i++)
   {
      directory_m[entryNum].Al[i] = entry[pos++];
   }

   directory_m[entryNum].linkEntry = -1;
}

void
CPM::printDirectoryEntry(uint8_t *entry)
{
   int pos = 0;

   printf("--------------------\n");
   printf("Status: 0x%02x", entry[pos]);
   if (entry[pos++] == 0xE5)
   {
      printf("\n");
      return;
   }
   printf("\n");

   for (int i = 0; i < 8; i++)
   {
      if (entry[pos + i] & 0x80)
      {
         printf("unexpected high bit set on filename pos %d\n", i);
      }    
   }
   if (entry[pos + 8] & 0x80)
   {
      printf("R/O ");
   }    
   if (entry[pos + 9] & 0x80)
   {
      printf("SYS ");
   }    
   if (entry[pos + 10] & 0x80)
   {
      printf("ARC");
   }
   printf("\n");

   for (int i = 0; i < 8; i++)
   {
      printf("%c", entry[pos++]);
   }
   printf(".");
   for (int i = 0; i < 3; i++)
   {
      printf("%c", entry[pos++]);
   }
   printf("\n");
   printf(" Xl = %d\n", entry[pos++]);
   printf(" Bc = %d\n", entry[pos++]);
   printf(" Xh = %d\n", entry[pos++]);
   printf(" Rc = %d\n", entry[pos++]);
   for (int i = 0; i < 16; i++)
   {
      printf(" Al = %d\n", entry[pos++]);
   }
}

void
CPM::dumpSector(uint16_t sectorNum)
{
#if CPMDEF
#else
    Sector *sector = getSector(sectorNum);
    uint8_t * data = sector->getSectorData();

    for(int i = 0; i < 256; i++)
    {
        printf("%c", data[i]);
    }
#endif
}

void
CPM::dumpCluster(uint8_t cluster, uint8_t lastSectorIndex)
{
#if CPMDEF
#else
    uint16_t sectorNum = cluster * diskInfo.spg;

    for (int i = 0; i < lastSectorIndex; i++)
    {
        dumpSector(sectorNum + i);
    }
     
#endif
}

uint8_t
CPM::saveSector(FILE *file, uint16_t sectorNum, uint8_t records)
{   
#if CPMDEF
    Sector *sector = getSector(sectorNum);
    uint8_t * data = sector->getSectorData();
    uint8_t error = sector->getErrorCode();
    if (records > 2) { records = 2; }
    fwrite(data, 128 * records, 1, file);

    return error;
#else
    Sector *sector = getSector(sectorNum);
    uint8_t * data = sector->getSectorData();
    uint8_t error = sector->getErrorCode();

    fwrite(data, 256, 1, file);

    return error;
#endif
}

/*
void
CPM::saveCluster(FILE * file,
                  uint8_t cluster,
                  uint8_t lastSectorIndex,
                  uint16_t &sizeInSectors,
                  uint16_t &badSectors)
{   
#if CPMDEF
#else
    uint16_t sectorNum = cluster * diskInfo.spg;
    
    for (int i = 0; i < lastSectorIndex; i++)
    {
        usedSectors_m++;
        sizeInSectors++;
        if (saveSector(file, sectorNum + i) > 0) 
        {
            badSectors++;
        }
    }
#endif
}
*/

void
CPM::saveBlock(FILE * file,
               uint8_t block,
               uint8_t numSectors,
               uint16_t &sizeInSectors,
               uint16_t &badSectors)
{  
    // 4 sectors in a block, 30 system blocks(3 system tracks with 10 sectors per track)
    // uint16_t sectorNum = block * 4 + (systemTracks_m * sectorsPerTrack_m);
   
    for (int i = 0; i < numSectors; i++)
    {
        //usedSectors_m++;
        sizeInSectors++;
        /*if (saveSector(file, sectorNum + i) > 0)
        {
            badSectors++;
        }*/
    }
}

void
CPM::dumpFile(uint8_t firstGroup, uint8_t lastGroup, uint8_t lastSectorIndex)
{
#if CPMDEF
#else
    int pos = firstGroup;
    int count = 0;
    printf("file data:\n----------------------\n");
    while ((pos != lastGroup) && (count++ < numClusters_m)) 
    {
        dumpCluster(pos, diskInfo.spg);
        pos = GRT[pos];
    }
    dumpCluster(pos, lastSectorIndex);
    printf("--------------------\nEnd file data\n"); 
    if (count > numClusters_m)
    {
        printf("\n----------------\n  Bad Chain - too many links\n---------------\n");
    }
    else if (GRT[pos] != 0)
    {
        printf("\n----------------\n  Bad Chain - last cluster not zero: %d\n---------------\n", GRT[pos]);
    }
#endif
}

uint16_t
CPM::saveFile(char* filename, uint8_t firstGroup, uint8_t lastGroup, uint8_t lastSectorIndex)
{
    uint16_t badSectors = 0;
#if CPMDEF
#else
    FILE *file = fopen(filename, "w");
    int pos = firstGroup;
    int count = 0;
    uint16_t sizeInSectors = 0;
#if SUMMARY
    while ((pos != lastGroup) && (count++ < numClusters_m))
    {
        saveCluster(file, pos, diskInfo.spg, sizeInSectors, badSectors);
        pos = GRT[pos];
    }
    saveCluster(file, pos, lastSectorIndex, sizeInSectors, badSectors);

    /*
    if (badSectors)
    {
        fprintf(indexFile_m, "   Bad sector count: %d\n", badSectors);
    } */
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
        saveCluster(file, pos, diskInfo.spg);
        pos = GRT[pos];
    }
    saveCluster(file, pos, lastSectorIndex);
    printf("--------------------\nEnd file data\n");
    if (count > numClusters_m)
    {
        printf("\n----------------\n  Bad Chain - too many links\n---------------\n");
    }
    else if (GRT[pos] != 0)
    {
        printf("\n----------------\n  Bad Chain - last cluster not zero: %d\n---------------\n", GRT[pos]);
    }
#endif
#endif

    return badSectors;
}

