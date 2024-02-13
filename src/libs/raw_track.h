//! \file raw_track.h
//!
//! Handles the raw data from fc5025
//!

#ifndef __RAW_TRACK_H__
#define __RAW_TRACK_H__

#include <vector>
#include <iostream>
#include <fstream>

class RawSector;

class RawTrack {

public:

    RawTrack(uint8_t   side,
             uint8_t   track);

    RawTrack(uint8_t  *buf,
             uint32_t  size,
             uint32_t &length);

    ~RawTrack();

    bool addRawSector(RawSector    *sector);
    bool writeToFile(std::ofstream &file);

    static const unsigned char headerSize_c = 7;

private:

    std::vector<RawSector *> sectors_m;
    uint8_t                  side_m;
    uint8_t                  track_m;
    uint32_t                 size_m;

};

#endif
