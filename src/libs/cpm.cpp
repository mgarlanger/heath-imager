//! \file cpm.cpp
//!
//! Handles CP/M disk images.
//!

#include "cpm.h"
#include "h17disk.h"
#include "h17block.h"

#include "sector.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

uint8_t h17DiskSkew[10] = { 0, 4, 8, 2, 6, 1, 5, 9, 3, 7 };

//!  Constructor
//!
//!  Defaults to single-sided, 40 tracks
//!
CPM::CPM(H17Disk* diskImage): diskImage_m(diskImage)
{
   H17DiskFormatBlock *diskFormat = (H17DiskFormatBlock *)
                diskImage_m->getH17Block(H17Block::DiskFormatBlock_c);

   // basically constants for hard-sectored disks
   systemTracks_m    = 3;
   sectorsPerTrack_m = 10;
   bytesPerSector_m  = 256;
   firstDataSector_m = systemTracks_m * sectorsPerTrack_m;
   fatalError_m = false;
   onlyUserZeroFiles_m = true; // assume true, until find other user files

   sides_m = diskFormat->getSides();
   tracks_m = diskFormat->getTracks();

   if (sides_m == 1)
   {
      if (tracks_m == 40)
      {
          // 100k
          blockSize_m = 4;
          directoryBlocks_m = 2;
          numBlocks_m = 92;
      }
      else
      {
          // 200k
          // 80 tracks
          // TODO determine this - not sure what the right parameters are
          blockSize_m = 4;
          directoryBlocks_m = 2;
          numBlocks_m = 192;
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
          numBlocks_m = 192;
      }
      else
      {
          // 400k
          // 80 tracks
          // TODO verify this - this seems right
          blockSize_m = 8;
          directoryBlocks_m = 2;
          numBlocks_m = 196;
      }
   }

   blockSizeInBytes_m = blockSize_m * bytesPerSector_m;
   freeBlocks_m = new bool[numBlocks_m];

   int i;

   // set directory blocks to used
   for(i = 0; i < directoryBlocks_m; i++) {
      freeBlocks_m[i] = false;
   }

   // set the rest to true, until the directory is processed
   for( ; i < numBlocks_m; i++) {
      freeBlocks_m[i] = true;
   }

   indexFile_m = fopen("0_index.info","w");

   if (!loadDiskInfo())
   {
       printf("Unable to load disk info\n");
       fatalError_m = true;
   }

   numFiles_m = 0;
}

CPM::~CPM()
{
   fclose(indexFile_m);
}

bool
CPM::isValidImage(H17Disk& diskImage)
{
    H17DataBlock   *diskData = (H17DataBlock *) diskImage.getH17Block(H17Block::DataBlock_c);
    H17DiskFormatBlock *diskFormat = (H17DiskFormatBlock *)
                diskImage.getH17Block(H17Block::DiskFormatBlock_c);

    uint8_t sides = diskFormat->getSides();
    uint8_t tracks = diskFormat->getTracks();

    return CPM::validateDirectory(diskData, sides, tracks);
}


// Check to see if the directory is in a valid format.
//
bool
CPM::validateDirectory(H17DataBlock *diskData, uint8_t sides, uint8_t tracks)
{

    uint8_t numDirSectors = 8;

    if (tracks == 80)
    {
        numDirSectors = 16;
    }

    uint8_t lastDirSector = 30 + numDirSectors;

    for(int dirSector = 30; dirSector < lastDirSector; dirSector++)
    {
        Sector *sector = getSector(diskData, sides, dirSector);
        if (!sector)
        {
            printf("unable to find sector - sides: %d dirSector: %d\n", sides, dirSector);
            continue;
        }

        uint8_t *data = sector->getSectorData();

        for (uint8_t entry = 0; entry < 8; entry++)
        {
            bool valid = CPM::validateDirectoryEntry(&data[entry*32]);

            if (!valid)
            {
                printf("Invalid Directory- Sector: %d entry: %d\n", dirSector, entry);
                return false;
            }
        }
    }

    return true;
}

bool
CPM::validateDirectoryEntry(uint8_t *entry)
{
   int pos = 0;
   DirectoryEntry de;

   // printf("--------------------\n");
   // printf("Status: 0x%02x", entry[pos]);
   de.userNumber = entry[pos++];

   if ((de.userNumber == 0xE5) ||
      // Dual Software Toolworks seem to not have 0xE5 for the rest of the directory entries
       (de.userNumber == 0x47) ||
       (de.userNumber == 0x4c))
   {
      // deleted entry, assume valid
      return true;
   }


   if (de.userNumber > 15) {
      // User number higher than 15, assume invalid
      return false;
   }

   // TODO add checks for valid filenames, allocation, etc.
   return true;

   /*
   for (int i = 0; i < 8; i++)
   {
      if (entry[pos + i] & 0x80)
      {
         printf("unexpected high bit set on filename pos %d\n", i);
      }
   }
   de->readOnly = (entry[pos + 8] & 0x80);
   de->systemFile = (entry[pos + 9] & 0x80);
   de->archived = (entry[pos + 10] & 0x80);

   uint8_t fullnamePos = 0;
   uint8_t keyFileNamePos = 0;

   de->keyFileName[keyFileNamePos++] = de->userNumber + '0';
   for (int i = 0; i < 8; i++)
   {
      char ch = entry[pos++] & 0x7f;

      de->fileName[i] = ch;
      if(ch != 32)
      {
          de->fullFileName[fullnamePos++] = ch;
          de->keyFileName[keyFileNamePos++] = ch;
      }
   }

   de->fullFileName[fullnamePos++] = '.';

   for (int i = 0; i < 3; i++)
   {
      char ch = entry[pos++] & 0x7f;

      de->fileExt[i] = ch;
      if(ch != 32)
      {
          de->fullFileName[fullnamePos++] = ch;
          de->keyFileName[keyFileNamePos++] = ch;
      }
   }
   de->fullFileName[fullnamePos] = 0;
   de->keyFileName[keyFileNamePos++] = 0;

   uint8_t Xl = entry[pos++];
   de->Xl = Xl;
   de->Bc = entry[pos++];
   uint8_t Xh = entry[pos++];
   de->Xh = Xh;
   de->Extent = (Xh << 5) | (Xl & 0x1f);
   de->Rc = entry[pos++];

   printf("File: %s, Extent: %d Rc: %d\n", de->fullFileName, de->Extent, de->Rc);

   printf("Alloc:");
   for (int i = 0; i < 16; i++)
   {
      uint16_t al = entry[pos++];
      printf(" %03d");
      de->Al[i] = al;
      if (al > 0) {
          freeBlocks_m[al] = false;
      }
   }
   printf("\n");

   de->linkEntry = -1;
   std::string keyFileName = std::string(de->keyFileName);

   directoryMap_m[de->userNumber].insert(std::pair<std::string, int>(keyFileName, entryNum) );

   std::map<std::string, FileBlock>::iterator it;
   std::string fileName = std::string(de->fullFileName);

   it = fileMap_m[de->userNumber].find(fileName);
   if (it == fileMap_m[de->userNumber].end())
   {
      // no existing entry, add it.
      FileBlock fb = {
         de->userNumber,
         fileName,
         1,
         de->Rc
      };

      for (int i = 0; i < 16; i++)
      {
         if (de->Al[i] > 0) {
             fb.allocBlocks.push_back(de->Al[i]);
         }
      }
      fileMap_m[de->userNumber].insert(std::pair<std::string, FileBlock>(fileName, fb));
   }
   else
   {
      FileBlock *fb = &it->second;


      if (de->Extent != fb->nextExpectedExtent)
      {
          printf("validate - not expected extent - expected: %d  saw: %d\n", fb->nextExpectedExtent, de->Extent);
      }
      else
      {
          printf("validate - expected extent - expected: %d  saw: %d\n", fb->nextExpectedExtent, de->Extent);

      }
      fb->nextExpectedExtent++;

      for (int i = 0; i < 16; i++)
      {
         if (de->Al[i] > 0) {
             fb->allocBlocks.push_back(de->Al[i]);
         }
      }
   } */
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

   for(uint16_t i = 0; i < directorySize_m; i++)
   {
       printEntry(i);
   }

   uint16_t numFreeBlocks = this->getFreeSpace();

   printf("FreeBlocks = %d, size = %d\n", numFreeBlocks, numFreeBlocks * 1024);

   printf("Name:entry\n");

   if (onlyUserZeroFiles_m) {
       printf("All user 0 files\n");
   }

   // std::string lastKey;
   /*
   for (uint8_t i = 0; i < maxUserNum_c; i++) {
      printf("User area %d\n", i);
      for(std::pair<std::string, int> entry : directoryMap_m[i] ) {
         int entryNum = entry.second;

         if (lastKey == entry.first) {
             printf("\t Entry: %d  Ext: %d\n", entryNum, directory_m[entryNum].Extent);
         }
         else {
             lastKey = entry.first;
             printf("file: %s\n\t Entry: %d  Ext: %d\n", directory_m[entryNum].fullFileName, entryNum, directory_m[entryNum].Extent);
         }

      }
   } */
   for (uint8_t i = 0; i < maxUserNum_c; i++) {
      printf("User area %d\n", i);
      for(std::pair<std::string, FileBlock> entry : fileMap_m[i] ) {
         FileBlock fileBlock = entry.second;

         std::string fileName = entry.first;

         printf("file: %s\n", fileName.c_str());
         printf("alloc blocks: ");

         std::vector<int> vect = fileBlock.allocBlocks;
         for (std::vector<int>::iterator vit = vect.begin() ; vit != vect.end(); ++vit)
         {
               printf(" %d", *vit);
         }
         printf("\n");
      }
   }

   return true;
}


// returns number of blocks
uint16_t
CPM::getFreeSpace()
{
   uint16_t space = 0;

   for(int i = 0; i < numBlocks_m; i++) {
       if (freeBlocks_m[i]) {
           space++;
       }
   }

   return space;
}

uint32_t
CPM::getFreeSpaceInBytes()
{
    uint32_t freeBlockCount = this->getFreeSpace();

    return freeBlockCount * blockSizeInBytes_m;
}

bool
CPM::loadDiskInfo()
{

   loadDirectory();

   return true;
}


Sector *
CPM::getSector(H17DataBlock *diskData, uint8_t sides, uint16_t sectorNum) {

    int sectorNo = sectorNum % 10;
    int physicalSector = h17DiskSkew[sectorNo];
    int sideNum;
    int trackNum;

    if (sides == 1)
    {
        sideNum = 0;
        trackNum = (sectorNum / 10);
    }
    else
    {
        sideNum = (sectorNum / 10) & 0x01;
        trackNum = (sectorNum / 20);
    }

    Sector *foundSector = diskData->getSector(sideNum, trackNum, physicalSector);

    if (!foundSector)
    {
        printf("getSector - sectorNum: %d, sectorNo: %d, physicalSector: %d, sideNum: %d, trackNum: %d\n", sectorNum, sectorNo, physicalSector, sideNum, trackNum);
    }

    return foundSector;
}

Sector *
CPM::getSector(uint16_t sectorNum) {

    H17DataBlock *diskData = (H17DataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);

    return CPM::getSector(diskData, sides_m, sectorNum);

/*
    H17DataBlock *diskData = (H17DataBlock *) diskImage_m->getH17Block(H17Block::DataBlock_c);

    int sectorNo = sectorNum % 10;
    int physicalSector = h17DiskSkew[sectorNo];
    int sideNum;
    int trackNum;

    if (sides_m == 1)
    {
        sideNum = 0;
        trackNum = (sectorNum / 10);
    }
    else
    {
        sideNum = (sectorNum / 10) & 0x01;
        trackNum = (sectorNum / 20);
    }

    return diskData->getSector(sideNum, trackNum, physicalSector);
*/
}


void
CPM::loadDirectory()
{
   uint16_t directoryEntry = 0;
   uint8_t numDirSectors = 8;

   // not sure if this is right.
   if (tracks_m == 80)
   {
       numDirSectors = 16;
   }

   uint8_t lastDirSector = 30 + numDirSectors;

   for(int dirSector = 30; dirSector < lastDirSector; dirSector++)

   {
       Sector *sector = getSector(dirSector);

       uint8_t *data = sector->getSectorData();

       for (uint8_t entry = 0; entry < 8; entry++)
       {
           loadDirectoryEntry(&data[entry*32], directoryEntry++);
       }
   }

   directorySize_m = directoryEntry;
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
   // TODO update for different sizes.
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
   DirectoryEntry *de = &directory_m[entryNum];

   // printf("--------------------\n");
   // printf("Status: 0x%02x", entry[pos]);
   de->userNumber = entry[pos++];

   if ((de->userNumber == 0xE5) ||
      // Dual Software Toolworks seem to not have 0xE5 for the rest of the directory entries
       (de->userNumber == 0x47) ||
       (de->userNumber == 0x4c))
   {
      de->deleted = true;
      return;
   }
   de->deleted = false;

   if (de->userNumber > 0) {
      onlyUserZeroFiles_m = false;
   }

   for (int i = 0; i < 8; i++)
   {
      if (entry[pos + i] & 0x80)
      {
         printf("unexpected high bit set on filename pos %d\n", i);
      }
   }
   de->readOnly = (entry[pos + 8] & 0x80);
   de->systemFile = (entry[pos + 9] & 0x80);
   de->archived = (entry[pos + 10] & 0x80);

   uint8_t fullnamePos = 0;
   uint8_t keyFileNamePos = 0;

   de->keyFileName[keyFileNamePos++] = de->userNumber + '0';
   for (int i = 0; i < 8; i++)
   {
      char ch = entry[pos++] & 0x7f;

      de->fileName[i] = ch;
      if(ch != 32)
      {
          de->fullFileName[fullnamePos++] = ch;
          de->keyFileName[keyFileNamePos++] = ch;
      }
   }

   de->fullFileName[fullnamePos++] = '.';

   for (int i = 0; i < 3; i++)
   {
      char ch = entry[pos++] & 0x7f;

      de->fileExt[i] = ch;
      if(ch != 32)
      {
          de->fullFileName[fullnamePos++] = ch;
          de->keyFileName[keyFileNamePos++] = ch;
      }
   }
   de->fullFileName[fullnamePos] = 0;
   de->keyFileName[keyFileNamePos++] = 0;

   uint8_t Xl = entry[pos++];
   de->Xl = Xl;
   de->Bc = entry[pos++];
   uint8_t Xh = entry[pos++];
   de->Xh = Xh;
   de->Extent = (Xh << 5) | (Xl & 0x1f);
   de->Rc = entry[pos++];

   printf("File: %s, Extent: %d Rc: %d Bc: %d\n", de->fullFileName, de->Extent, de->Rc, de->Bc);
   printf("Alloc:");
   for (int i = 0; i < 16; i++)
   {
      uint16_t al = entry[pos++];
      printf(" %03d", al);
      de->Al[i] = al;
      if (al > 0) {
          freeBlocks_m[al] = false;
      }
   }
   printf("\n");

   de->linkEntry = -1;
   std::string keyFileName = std::string(de->keyFileName);

   directoryMap_m[de->userNumber].insert(std::pair<std::string, int>(keyFileName, entryNum) );

   std::map<std::string, FileBlock>::iterator it;
   std::string fileName = std::string(de->fullFileName);

   it = fileMap_m[de->userNumber].find(fileName);
   if (it == fileMap_m[de->userNumber].end())
   {
      uint16_t records = de->Rc;
      if (de->Extent != 0)
      {
         records += (de->Extent) * 128;
      }
      // no existing entry, add it.
      FileBlock fb = {
         de->userNumber,
         fileName,
         (uint16_t) (de->Extent + 1),
         records
      };

      for (int i = 0; i < 16; i++)
      {
         if (de->Al[i] > 0) {
             fb.allocBlocks.push_back(de->Al[i]);
         }
      }
      fileMap_m[de->userNumber].insert(std::pair<std::string, FileBlock>(fileName, fb));
   }
   else
   {
      FileBlock *fb = &it->second;

      uint16_t records = de->Rc;

      if (de->Extent != fb->nextExpectedExtent)
      {
          //printf("load - not expected extent - expected: %d  saw: %d\n", fb->nextExpectedExtent, de->Extent);
          records += (de->Extent - fb->nextExpectedExtent) * 128;
      }
      fb->nextExpectedExtent++;

      fb->records += de->Rc;

      for (int i = 0; i < 16; i++)
      {
         if (de->Al[i] > 0) {
             fb->allocBlocks.push_back(de->Al[i]);
         }
      }
   }
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
      printf("%c", entry[pos++] & 0x7f);
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
CPM::dumpSector(uint16_t sectorNum, uint8_t numRecords)
{
    Sector *sector = getSector(sectorNum);
    uint8_t * data = sector->getSectorData();

    if (numRecords > 2) {
        numRecords = 2;
    }
    for(int i = 0; i < numRecords * 128; i++)
    {
        printf("%c", data[i]);
    }

}

uint8_t
CPM::saveSector(FILE *file, uint16_t sectorNum, uint8_t records)
{
    Sector *sector = getSector(sectorNum);

    if (!sector)
    {
        printf("sector not found: %d\n", sectorNum);
        return 1;

    }
    uint8_t * data = sector->getSectorData();
    uint8_t error = sector->getErrorCode();
    int recordsToWrite = (records > 2) ? 2 : records;

    fwrite(data, 128, recordsToWrite, file);

    return error;
}


void
CPM::saveBlock(FILE *    file,
               uint8_t   block,
               uint16_t  numRecords,
               uint16_t &sizeInRecords,
               uint16_t &badSectors)
{
    // 4 sectors in a block, 30 system blocks(3 system tracks with 10 sectors per track)

    uint16_t sectorNum = block * blockSize_m + (systemTracks_m * sectorsPerTrack_m);

    uint16_t recordsInBlock = blockSize_m * 2;

    int recordsToWrite = (numRecords > recordsInBlock) ? recordsInBlock : numRecords;

    // records are 128 bytes
    while (recordsToWrite > 0)
    {

        if (saveSector(file, sectorNum++, recordsToWrite) > 0)
        {
            badSectors++;
        }

        recordsToWrite -= 2;
    }

}

bool
CPM::saveFile(FileBlock fileblock)
{
    FILE *file = fopen(fileblock.fileName.c_str(), "w");

    if (!file)
    {
        printf("Unable to create file: %s\n", fileblock.fileName.c_str());
        return false;
    }

    std::vector<int> vect = fileblock.allocBlocks;
    uint16_t records = fileblock.records;

    uint16_t sizeInRecords;
    uint16_t badSectors = 0;
    printf("name: %s  records: %d\n", fileblock.fileName.c_str(), records);

    for (std::vector<int>::iterator vit = vect.begin() ; vit != vect.end(); ++vit)
    {
          saveBlock(file, *vit, records, sizeInRecords, badSectors);
          records -= blockSize_m * 2;
    }
    if (badSectors != 0)
    {
        printf(" -- number of bad sectors: %d\n", badSectors);
        fprintf(indexFile_m, " -- number of bad sectors: %d\n", badSectors);
    }

    fclose(file);

    return true;
}

bool
CPM::listFile(FileBlock fileblock)
{
    int sizeInKB = (int) ((fileblock.allocBlocks.size() * blockSizeInBytes_m + 1023) / 1024);

    fprintf(indexFile_m, "  %-12s  %3dK\n", fileblock.fileName.c_str(), sizeInKB);

    return true;
}


bool
CPM::saveAllFiles()
{

   /**   listFiles();

   for (uint8_t i = 0; i < maxUserNum_c; i++)
   {
      if (fileMap_m[i].size() > 0)
      {
         // let group zero be in main directory, create if not zero.
         if (i > 0)
         {
            fprintf(indexFile_m, "User %02d:\n", i);
         }

         for(std::pair<std::string, FileBlock> entry : fileMap_m[i] )
         {
            FileBlock fileBlock = entry.second;

            listFile(fileBlock);
         }
      }
   }

   fprintf(indexFile_m, "Free space:    %4dK\n", (int) ((getFreeSpaceInBytes() + 1023) / 1024));
   **/

   for (uint8_t i = 0; i < maxUserNum_c; i++)
   {
      if (fileMap_m[i].size() > 0)
      {
         // let group zero be in main directory, create if not zero.
         if (i > 0)
         {
            char directoryName[3];
            snprintf(directoryName, 3, "%02d", i);
            mkdir(directoryName, 0777);
            chdir(directoryName);

            fprintf(indexFile_m, "User %02d:\n", i);
         }

         printf("User area %d\n", i);
         for(std::pair<std::string, FileBlock> entry : fileMap_m[i] )
         {
            FileBlock fileBlock = entry.second;

            listFile(fileBlock);
            saveFile(fileBlock);
         }
         if (i > 0)
         {
            chdir("..");
         }
      }
   }

   fprintf(indexFile_m, "Free space:    %4dK\n", (int) ((getFreeSpaceInBytes() + 1023) / 1024));

   return true;
}

bool
CPM::listFiles()
{

   for (uint8_t i = 0; i < maxUserNum_c; i++)
   {
      if (fileMap_m[i].size() > 0)
      {
         // let group zero be in main directory, create if not zero.
         if (i > 0)
         {
            fprintf(indexFile_m, "User %02d:\n", i);
         }

         for(std::pair<std::string, FileBlock> entry : fileMap_m[i] )
         {
            FileBlock fileBlock = entry.second;

            listFile(fileBlock);
         }
      }
   }

   fprintf(indexFile_m, "Free space:    %4dK\n", (int) ((getFreeSpaceInBytes() + 1023) / 1024));

   return true;
}


