//! \file sector.h
//!
//! Handles sectors in the h17disk file format.
//!

#ifndef __SECTOR_H__
#define __SECTOR_H__

#include <iostream>
#include <fstream>
#include <cstdint>


class Sector
{
public:

    Sector(uint8_t  *buf,
           uint16_t  size,
           uint16_t &length);

    Sector(uint8_t   side,
           uint8_t   track,
           uint8_t   sector,
           uint8_t   error,
           uint8_t  *buf,
           uint16_t  bufSize);

    ~Sector();

    bool     writeToFile(std::ofstream &file);
    bool     writeToH8D(std::ofstream &file);
    bool     writeToRaw(std::ofstream &file);

    uint8_t  getSectorNum();
    uint16_t getBlockSize();
    bool     analyze();

    uint16_t getSectorHeaderOffset();
    uint16_t getSectorDataOffset();
    uint8_t *getSectorData();
    uint8_t  getErrorCode();

    static const uint8_t headerSize_c = 5;

    bool     dump(int level);

private:

    void dumpHeader(unsigned char buf[]);
    void dumpData(unsigned char buf[]);

    uint16_t  bufSize_m;
    uint8_t  *buf_m;
    uint8_t   sector_m;
    uint8_t   error_m;

};

#endif
