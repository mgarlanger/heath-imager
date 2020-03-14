//! \file raw_sector.h
//!
//! Handles the raw data from fc5025
//!
//!
#ifndef __RAW_SECTOR_H__
#define __RAW_SECTOR_H__

#include <iostream>
#include <fstream>
#include <cstdint>

class RawSector
{

public:

    RawSector(uint8_t   side,
              uint8_t   track,
              uint8_t   sector,
              uint8_t  *buf,
              uint16_t  bufSize);

    RawSector(uint8_t  *buf,
              uint32_t  size,
              uint16_t  &length);

    ~RawSector();

    bool     writeToFile(std::ofstream &file);
    uint16_t getBufSize();
    uint16_t getBlockSize();
    void     dumpSector();

    static const uint8_t headerSize_c = 4;

private:

    uint16_t   bufSize_m;
    uint8_t   *buf_m;
    uint8_t    sector_m;

};

#endif

