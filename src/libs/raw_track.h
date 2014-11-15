#ifndef __RAW_TRACK_H__
#define __RAW_TRACK_H__

#include <vector>
#include <iostream>
#include <fstream>

class RawSector;

class RawTrack {

public:
    RawTrack(unsigned char  side,
             unsigned char  track);
    ~RawTrack();

    bool addRawSector(RawSector *sector);

    bool writeToFile(std::ofstream &file);

    static const unsigned char headerSize_c = 7;

private:

    std::vector<RawSector *> sectors_m;
    unsigned char  side_m;
    unsigned char  track_m;

};

#endif

