//! \file raw_sector.cpp
//!
//! Handles the raw data from fc5025
//!

#include "raw_sector.h"
#include "h17disk.h"

#include <string.h>

//! constructor - for when only sector data is available 
//!
//! @param side
//! @param track
//! @param sector
//! @param buf
//! @param bufSize
//!
RawSector::RawSector(uint8_t    side,
                     uint8_t    track,
                     uint8_t    sector,
                     uint8_t   *buf,
                     uint16_t   bufSize): bufSize_m(bufSize),
                                          // side_m(side),
                                          // track_m(track),
                                          sector_m(sector)
{
    buf_m = new uint8_t[bufSize_m];
    memcpy(buf_m, buf, bufSize);
}


//! constructor - for decoding an existing file.
//!
//! @param buf
//! @param size
//! @param length
//!
RawSector::RawSector(uint8_t  *buf,
                     uint32_t  size,
                     uint16_t &length)
{
     if (buf[0] == H17Disk::RawSectorDataId)
     {
         sector_m =  buf[1];
         bufSize_m = (buf[2] << 8) | buf[3];
         // TODO make sure bufSize_m isn't larger than size
         //
         buf_m = (uint8_t*) new char[bufSize_m];
         memcpy(buf_m, &buf[4], bufSize_m);

         length = bufSize_m + 4;
     }
     else
     {
         //*** error
         printf("Unexpected byte instead of SectorDataId: %d\n", buf[0]);
     }
}
         

//! destructor
//!
RawSector::~RawSector()
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    if (buf_m)
    {
        delete[] buf_m;
    }
}


//!
//! write data to a file in h17disk format
//!
//! @param file - file to write to
//!
//! @return success
//!
bool
RawSector::writeToFile(std::ofstream &file)
{
    uint8_t header[headerSize_c] = { 
        H17Disk::RawSectorDataId, 
        sector_m, 
        (uint8_t) ((bufSize_m >> 8) & 0xff), 
        (uint8_t) (bufSize_m & 0xff)
    };

    file.write((const char*) header, headerSize_c);
    file.write((const char*) buf_m, bufSize_m);

    return true;
}


//! Get buffer size
//!
//! @return buffer size
//!
uint16_t
RawSector::getBufSize()
{
    return bufSize_m;
}


//! Get entire size of block including header
//!
//! @return block size
//!
uint16_t
RawSector::getBlockSize()
{
    return bufSize_m + headerSize_c;
}


//! dump sector
//!
void
RawSector::dumpSector()
{

}
