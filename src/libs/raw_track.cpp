
#include "raw_track.h"
#include "raw_sector.h"

RawTrack::RawTrack(unsigned char  side,
                   unsigned char  track): side_m(side),
                                          track_m(track)
{
    sectors_m.reserve(100);
}


RawTrack::~RawTrack()
{
    
}

bool RawTrack::addRawSector(RawSector *sector)
{
    sectors_m.push_back(sector);

    return true;
}

bool RawTrack::writeToFile(std::ofstream &file)
{
    uint32_t size = 0;
 
    /// \todo loop through all the sectors and get actual size of each
    if (sectors_m.size())
    {
        size = sectors_m.size() * (sectors_m[0]->getBufSize() + 4);
    }

    uint8_t buf[7] = { 
                         0x31,
                         side_m,
                         track_m, 
                         (unsigned char) ((size >> 24) & 0xff),
                         (unsigned char) ((size >> 16) & 0xff),
                         (unsigned char) ((size >> 8) & 0xff),
                         (unsigned char) (size & 0xff)
                     };

    file.write((const char*) buf, 7);

    for (unsigned int i = 0; i < sectors_m.size(); i++)
    {
        sectors_m[i]->writeToFile(file);
    }

    return true;
}

