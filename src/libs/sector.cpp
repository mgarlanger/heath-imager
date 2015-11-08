//! \file sector.cpp
//!
//! Handles the sector
//!

#include "sector.h"
#include "h17disk.h"
#include "disk_util.h"


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
                                   side_m(side),
                                   track_m(track),
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
    printf("%s\n", __PRETTY_FUNCTION__);
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


//! write sector to file
//!
//! @param file    file to write to
//!
//! @return success
//!
bool
Sector::writeToH8D(std::ofstream &file)
{

    uint16_t pos = 0;
  
    // look for the sync for the header 
    while ((buf_m[pos] != 0xfd) && (pos < bufSize_m))
    {
        pos++;
    }

    // skip past the header, since 0xfd could be the checksum
    pos += 5;

    // look for the data 
    while ((buf_m[pos] != 0xfd) && (pos < bufSize_m))
    {
        pos++;
    }

    if ((bufSize_m - pos) < 256)
    {
        printf("Error data not found - sector: %d\n", sector_m);
        pos = bufSize_m - 256;
    }

    // write out the sector
    if (buf_m)
    {
        file.write((const char*) &buf_m[pos], 256);
    }

    return true;
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
    while ((buf_m[pos] != 0xfd) && (pos < bufSize_m))
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
        printf("sector(%d) has error: %s(%d)\n", sector_m, errorStrings[error_m], error_m);
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

