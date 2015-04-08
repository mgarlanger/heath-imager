
#include "sector.h"

#include <string.h>


// Sector constructor
//
// @param side      side nubmer for this sector
// @param track     track number for this sector
// @param sector    physical sector number
// @param error     error code for reading this sector
// @param buf       pointer to the sector data
// @param bufSize   size of the sector data
//
Sector::Sector(unsigned char  side,
               unsigned char  track,
               unsigned char  sector,
               unsigned char  error,
               unsigned char *buf,
               unsigned int   bufSize): bufSize_m(bufSize),
                                        buf_m(nullptr),
                                        side_m(side),
                                        track_m(track),
                                        sector_m(sector),
                                        error_m(error)
{
    // deep copy the sector data
    if ((buf) && (bufSize > 0))
    {
        buf_m = (unsigned char*) new char[bufSize];
        memcpy(buf_m, buf, bufSize);
    }
    else
    {
        bufSize = 0;
    }
}


// Sector destructor
//
Sector::~Sector()
{
    // free allocated memory
    if (buf_m)
    {
        delete[] buf_m;
    }
}


// writeToFile
//
// @param file    file to write to
//
// @return {bool} success
bool Sector::writeToFile(std::ofstream &file)
{
    // create header
    unsigned char buf[headerSize_c] = { 0x12,
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

