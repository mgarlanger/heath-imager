
#include "raw_track.h"
#include "raw_sector.h"

// Constructor RawTrack
//
// @param side    floppy disk side for this track
// @param track   track number
//
RawTrack::RawTrack(unsigned char  side,
                   unsigned char  track): side_m(side),
                                          track_m(track)
{
    sectors_m.reserve(100);
}


// Destructor
//
RawTrack::~RawTrack()
{
    
}


// addRawSector 
//
// @param sector     pointer to raw sector
//
// @return  {bool} success 
//
bool RawTrack::addRawSector(RawSector *sector)
{
    sectors_m.push_back(sector);

    return true;
}


// writeToFile
//
// writes the raw track to a file
//
// @param file   ofstream for file
//
// @return {bool} success
//
bool RawTrack::writeToFile(std::ofstream &file)
{
    uint32_t size = 0;

    // determine each sector's size 
    for (unsigned int i = 0; i < sectors_m.size(); i++)
    {
        size += sectors_m[i]->getBlockSize();
    }

    // generate the header
    uint8_t buf[headerSize_c] = { 
                                    0x31,
                                    side_m,
                                    track_m, 
                                    (unsigned char) ((size >> 24) & 0xff),
                                    (unsigned char) ((size >> 16) & 0xff),
                                    (unsigned char) ((size >> 8) & 0xff),
                                    (unsigned char) (size & 0xff)
                                };

    // write header
    file.write((const char*) buf, headerSize_c);

    // write each sector
    for (unsigned int i = 0; i < sectors_m.size(); i++)
    {
        sectors_m[i]->writeToFile(file);
    }

    return true;
}

