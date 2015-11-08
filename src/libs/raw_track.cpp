//! \file raw_track.cpp
//!
//! Handles the raw data from fc5025
//!

#include "raw_track.h"
#include "raw_sector.h"
#include "h17disk.h"


//! Constructor RawTrack
//!
//! @param side    floppy disk side for this track
//! @param track   track number
//!
RawTrack::RawTrack(uint8_t side,
                   uint8_t track): side_m(side),
                                   track_m(track)
{
    sectors_m.reserve(100);
}


//! constructor if data block already exists
//!
//! @param[in] buf - data buffer
//! @param[in] size - size of buffer
//! @param[out] length - total block length
//!
RawTrack::RawTrack(uint8_t  *buf,
                   uint32_t  size,
                   uint32_t &length)
{
    printf("Track::Track buf[0]: %d\n", buf[0]);
    if (H17Disk::RawTrackDataId == buf[0])
    {
        printf("In the true part\n");
        side_m =  buf[1];
        track_m = buf[2];
        size_m = (buf[3] << 24) | (buf[4] << 16) | (buf[5] << 8) | buf[6];
        uint32_t pos = 0;
        uint16_t cur_length;

        while(pos < size_m)
        {
            printf("Track::Track pos: %d  buf[pos]: %d\n", pos, buf[pos + 5]);
            sectors_m.push_back(new RawSector(&buf[pos + 7], size_m - pos, cur_length));
            pos += cur_length;
        }
        length = size_m + 7;
    }
    else
    {
        //*** error
        printf("Unexpected byte instead of RawTrackDataId: %d\n", buf[0]);
    }
}


//! Destructor
//!
RawTrack::~RawTrack()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    for(unsigned int i = 0; i < sectors_m.size(); ++i)
    {
        delete sectors_m[i];
    } 
}


//! addRawSector 
//!
//! @param sector     pointer to raw sector
//!
//! @return  success 
//!
bool
RawTrack::addRawSector(RawSector *sector)
{
    sectors_m.push_back(sector);

    return true;
}


//! writeToFile
//!
//! writes the raw track to a file
//!
//! @param file   ofstream for file
//!
//! @return  success
//!
bool
RawTrack::writeToFile(std::ofstream &file)
{
    uint32_t size = 0;

    // determine each sector's size 
    for (unsigned int i = 0; i < sectors_m.size(); i++)
    {
        size += sectors_m[i]->getBlockSize();
    }

    // generate the header
    uint8_t buf[headerSize_c] = { 
                                    H17Disk::RawTrackDataId,
                                    side_m,
                                    track_m, 
                                    (uint8_t) ((size >> 24) & 0xff),
                                    (uint8_t) ((size >> 16) & 0xff),
                                    (uint8_t) ((size >> 8)  & 0xff),
                                    (uint8_t)  (size        & 0xff)
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

