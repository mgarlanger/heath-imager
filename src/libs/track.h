//! \file raw_track.h
//!
//! Handles the raw data from fc5025
//!

#ifndef __TRACK_H__
#define __TRACK_H__

#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>


class Sector;

class Track {

public:

    Track(uint8_t  *buf,
          uint32_t size,
          uint32_t &length);

    Track(uint8_t  side,
          uint8_t  track);

    ~Track();

    bool addSector(Sector *sector);
    bool writeToFile(std::ofstream &file);
    bool analyze(bool validTracks[2][80]);
    bool writeH8D(std::ofstream &file);
    bool writeRaw(std::ofstream &file);

    static const uint8_t headerSize_c = 5;

private:

    std::vector<Sector *> sectors_m;
    uint8_t               side_m;
    uint8_t               track_m;
    uint16_t              size_m;

};

#endif

