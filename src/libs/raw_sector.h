#ifndef __RAW_SECTOR_H__
#define __RAW_SECTOR_H__

#include <iostream>
#include <fstream>
#include <cstdint>

class RawSector
{
public:
    RawSector(unsigned char  side,
              unsigned char  track,
              unsigned char  sector,
              unsigned char *buf,
              unsigned int   bufSize);
    ~RawSector();

    bool writeToFile(std::ofstream &file);
    unsigned int getBufSize();
    unsigned int getBlockSize();
    void dumpSector();

    static const uint8_t headerSize_c = 4;

private:
    unsigned int   bufSize_m;
    unsigned char *buf_m;
    unsigned char  side_m;
    unsigned char  track_m;
    unsigned char  sector_m;

};

#endif

