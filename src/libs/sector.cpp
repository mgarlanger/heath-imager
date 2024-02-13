//! \file sector.cpp
//!
//! Handles the hard-sectored disk sector
//!

#include "sector.h"
#include "h17disk.h"
#include "disk_util.h"
#include "dump.h"

#include <string.h>


//! Sector constructor
//!
//! @param side      side nubmer for this sector
//! @param track     track number for this sector
//! @param sector    physical sector number
//! @param error     error code for reading this sector
//! @param buf       pointer to the sector data
//! @param bufSize   size of the sector data
//!
Sector::Sector(uint8_t   side,
               uint8_t   track,
               uint8_t   sector,
               uint8_t   error,
               uint8_t  *buf,
               uint16_t  bufSize): bufSize_m(bufSize),
                                   buf_m(nullptr),
                                   sector_m(sector),
                                   error_m(error)
{
    // deep copy the sector data
    if ((buf) && (bufSize > 0))
    {
        buf_m = (uint8_t*) new char[bufSize];
        memcpy(buf_m, buf, bufSize);
    }
    else
    {
        buf = nullptr;
        bufSize = 0;
    }
}


//! Sector constructor
//!
//! @param buf    existing sector written value.
//! @param size   length of the buffer
//! @param length amount of space used by this sector
//!
Sector::Sector(uint8_t  *buf,
               uint16_t size,
               uint16_t &length)
{
     if (buf[0] == H17Disk::SectorDataId)
     {
         sector_m =  buf[1];
         error_m = buf[2];
         bufSize_m = (buf[3] << 8) | buf[4];

         buf_m = (uint8_t*) new char[bufSize_m]; 
         memcpy(buf_m, &buf[5], bufSize_m);

         length = bufSize_m + 5;
     }
     else
     {
         //*** error
         printf("Unexpected byte instead of SectorDataId: %d\n", buf[0]);
     }
}


//! Sector destructor
//!
Sector::~Sector()
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    // free allocated memory
    if (buf_m)
    {
        delete[] buf_m;
    }
}


//! write sector to file
//!
//! @param file    file to write to
//!
//! @return success
//!
bool
Sector::writeToFile(std::ofstream &file)
{
    // create header
    unsigned char buf[headerSize_c] = { H17Disk::SectorDataId,
                                        sector_m,
                                        error_m,
                                        (unsigned char) ((bufSize_m >> 8) & 0xff), 
                                        (unsigned char) (bufSize_m & 0xff) };

    // write header to disk
    file.write((const char*) buf, headerSize_c);

    // write out the sector
    if (buf_m)
    {
        file.write((const char*) buf_m, bufSize_m);
    }

    return true;
}

uint8_t
Sector::getErrorCode()
{
    return error_m;
}

uint16_t
Sector::getSectorHeaderOffset()
{
    uint16_t pos = 0;

    // look for the sync for the header
    while ((pos < bufSize_m) && (buf_m[pos++] != 0xfd))
    { }

    if ((bufSize_m - pos) < 5)
    {
        printf("Error header not found - sector: %d\n", sector_m);
        pos = bufSize_m - 5;
    }
    return pos;
}


uint16_t
Sector::getSectorDataOffset()
{
    uint16_t pos = 0;
 
    // look for the sync for the header
    while ((pos < bufSize_m) && (buf_m[pos] != 0xfd))
    {
        pos++;
    }

    // skip past the header, since 0xfd could be the checksum
    pos += 5;

    // look for the data
    while ((buf_m[pos++] != 0xfd) && (pos < bufSize_m))
    { }

    if ((bufSize_m - pos) < 256)
    {
        printf("Error data not found - sector: %d\n", sector_m);
        pos = bufSize_m - 256;
    }

    return pos;
}

//! write sector to file
//!
//! @param file    file to write to
//!
//! @return success
//!
bool
Sector::writeToH8D(std::ofstream &file)
{

    uint16_t pos = getSectorDataOffset();

    // write out the sector
    if (buf_m)
    {
        file.write((const char*) &buf_m[pos], 256);
    }

    return true;
}

uint8_t *
Sector::getSectorData()
{
    uint16_t pos = getSectorDataOffset();

    return &buf_m[pos];
}

//! write sector to file
//!
//! @param file    file to write to
//!
//! @return success
//!
bool
Sector::writeToRaw(std::ofstream &file)
{   
    
    // write out the sector
    if (buf_m)
    {
        file.write((const char*) buf_m, 320);
    }

    return true;
}


//! get sector number from the header portion of the sector
//!
//! @return success
//!
uint8_t
Sector::getSectorNum()
{

    uint16_t pos = 0;

    // look for the sync for the header 
    while ((pos < bufSize_m) && (buf_m[pos] != 0xfd))
    {
        pos++;
    }


    if (pos > (bufSize_m - 3))
    {
        printf("Could not find sector number: %d\n", sector_m);
        return 20;
    }
    {
        return buf_m[pos+3]; 
    }
    
}

//! analyze the sector
//!
//! @return success if sector has no error.
//!
bool 
Sector::analyze()
{
    bool valid = true;

    if (error_m)
    {
        valid = false;
        printf("sector(%d) has error: %s(%d)\n", sector_m, sectorErrorStrings[error_m], error_m);
    }
    if (sector_m >= 10)
    {
        valid = false;
        printf("sector out of range: %d\n", sector_m);
    }

    if (bufSize_m < 300)
    {
        valid = false;
        printf("size is too small: %d\n", bufSize_m);
    }

    return valid;
}


//! return size of sector including header
//!
//! @return size
//!
uint16_t
Sector::getBlockSize()
{
    return bufSize_m + headerSize_c;
}


bool
Sector::dump(int level)
{
    printf("        Sector:   %d\n", sector_m);
    if (error_m)
    {   
        printf("        Error:    %d - %s\n", error_m, sectorErrorStrings[error_m]);
    }
    else
    {   
        printf("        Sector Valid\n");
    }

    printf("      Data:---------------------------------------------------------------\n");
    dumpDataBlock(buf_m, bufSize_m);
    unsigned int pos;

    printf("        ------------------------------------------------------------------\n");
   
    // dump actual sector data.
    pos = 4;
    int headerPos = -1;
    int dataPos = -1;

    // Find header block
    while ((pos < bufSize_m) && (buf_m[pos] != 0xfd))
    {   
        pos++;
    }

    if (pos < bufSize_m)
    {   
        headerPos = pos;
    }
    // skip header.
    pos += 5;

    // find Data block
    while ((pos < bufSize_m) && (buf_m[pos] != 0xfd))
    {   
        pos++;
    }

    if (pos < bufSize_m)
    {
        dataPos = pos;
    }

    // if header found display it.
    if ((headerPos != -1) && (headerPos < ((int) bufSize_m - 5)))
    {
         dumpHeader(&buf_m[headerPos]);
    }
    else
    {
        printf("  sector header missing - headerPos: %d\n", headerPos);
    }

    // if data found display it
    if ((dataPos != -1) && (dataPos < ((int) bufSize_m - 258)))
    {
        dumpData(&buf_m[dataPos+1]);
    }
    else
    {
        printf("  sector data missing - dataPos: %d\n", dataPos);
    }

    return true;
}


//! dump SectorHeader
//!
//! @param      buf     data buffer
//!
void
Sector::dumpHeader(unsigned char buf[])
{
    uint8_t calculatedChecksum = 0;

    printf("    Sector Header\n");
    printf("       Volume: %3d\n", buf[1]);
    calculatedChecksum = updateChecksum(calculatedChecksum, buf[1]);
    printf("       Track:   %2d\n", buf[2]);
    calculatedChecksum = updateChecksum(calculatedChecksum, buf[2]);
    printf("       Sector:   %d\n", buf[3]);
    calculatedChecksum = updateChecksum(calculatedChecksum, buf[3]);
    if (buf[4] == calculatedChecksum)
    {
        printf("       Chksum: 0x%02x\n", buf[4]);
    }
    else
    {
        printf("       Chksum: 0x%02x (expected 0x%02x)\n", calculatedChecksum, buf[4]);
    }
}


//! dump SectorData
//!
//! @param      buf     data buffer
//!
void
Sector::dumpData(unsigned char buf[])
{
    printf("    Sector Data:\n");
    uint8_t printAble[16];
    uint8_t calculatedChecksum = 0;


    for (unsigned int i = 0; i < 256; i++)
    {
        calculatedChecksum = updateChecksum(calculatedChecksum, buf[i]);
        printAble[i % 16] = isprint(buf[i]) ? buf[i] : '.';
        if  ((i % 16) == 0)
        {
            printf("        %03d: ", i);
        }
        printf("%02x", buf[i]);

        if ((i % 16) == 7)
        {
            printf(" ");
        }
        if ((i % 16) == 15)
        {
            printf("        |");
            for(int i = 0; i < 8; i++)
            {
                printf("%c", printAble[i]);
            }
            printf("  ");
            for(int i = 8; i < 16; i++)
            {
                printf("%c", printAble[i]);
            }

            printf("|\n");
        }
    }

    if (buf[256] == calculatedChecksum)
    {
        printf("       Valid Data Chksum: 0x%02x\n", buf[256]);
    }
    else
    {
        printf("       Chksum: 0x%02x (expected 0x%02x)\n", calculatedChecksum, buf[256]);
    }
}
