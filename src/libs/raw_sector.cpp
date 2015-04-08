#include "raw_sector.h"

#include <string.h>

RawSector::RawSector(unsigned char  side,
                     unsigned char  track,
                     unsigned char  sector,
                     unsigned char *buf,
                     unsigned int   bufSize): bufSize_m(bufSize),
                                              side_m(side),
                                              track_m(track),
                                              sector_m(sector)
{
    buf_m = new unsigned char[bufSize_m];
    memcpy(buf_m, buf, bufSize);
}

RawSector::~RawSector()
{
    if (buf_m)
    {
        delete[] buf_m;
    }
}

bool RawSector::writeToFile(std::ofstream &file)
{
    uint8_t header[headerSize_c] = { 
                                        0x32, 
                                        sector_m, 
                                        (uint8_t) ((bufSize_m >> 8) & 0xff), 
                                        (uint8_t) (bufSize_m & 0xff)
                                    };

    file.write((const char*) header, headerSize_c);
    file.write((const char*) buf_m, bufSize_m);
    return true;
}


unsigned int RawSector::getBufSize()
{
    return bufSize_m;
}

unsigned int RawSector::getBlockSize()
{
    return bufSize_m + headerSize_c;
}

void RawSector::dumpSector()
{

}

