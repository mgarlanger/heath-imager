
#include "sector.h"

#include <string.h>

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


Sector::~Sector()
{
    if (buf_m)
    {
        delete[] buf_m;
    }
}

bool Sector::writeToFile(std::ofstream &file)
{
    unsigned char buf[headerSize_c] = { 0x12,
                                        sector_m,
                                        error_m,
                                        (unsigned char) ((bufSize_m >> 8) & 0xff), 
                                        (unsigned char) (bufSize_m & 0xff) };

    file.write((const char*) buf, headerSize_c);

    if (buf_m)
    {
        file.write((const char*) buf_m, bufSize_m);
    }

    return true;
}

