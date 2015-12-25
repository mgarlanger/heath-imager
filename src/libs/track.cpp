//! \file track.cpp
//!
//! Handles h17 hard sector track
//!

#include "track.h"
#include "sector.h"
#include "h17disk.h"

#include "h17block.h"


//! Constructor Track
//!
//! @param side    floppy disk side for this track
//! @param track   track number
//!
Track::Track(uint8_t side,
             uint8_t track): side_m(side),
                             track_m(track)
{
    sectors_m.reserve(100);
}


//! Constructor
//!
//! @param[in]  buf    buffer with existing track data
//! @param[in]  size   buffer size
//! @param[out] length amount of buffer used for track data
//!
Track::Track(uint8_t  *buf,
             uint32_t  size,
             uint32_t &length)
{
     if (H17Block::TrackDataId == buf[0])
     {
         side_m =  buf[1];
         track_m = buf[2];
         size_m = (buf[3] << 8) | buf[4];
         uint16_t pos = 0;
         uint16_t cur_length;

         while(pos < size_m)
         {
             sectors_m.push_back(new Sector(&buf[pos + 5], size_m - pos, cur_length));
             pos += cur_length;
         }
         length = size_m + 5;
     }
     else
     {
         //*** error
         printf("Unexpected byte instead of TrackDataId: %d\n", buf[0]);
     }
}


//! Destructor
//!
Track::~Track()
{
    for (unsigned int i = 0; i < sectors_m.size(); ++i)
    {
        delete sectors_m[i];
    }
    
}


//! addSector 
//!
//! @param sector     pointer to raw sector
//!
//! @return   success 
//!
bool
Track::addSector(Sector *sector)
{
    sectors_m.push_back(sector);

    return true;
}


//! analyze and validate track
//!
//! @param validTracks 
//!
//! @return if its valid
//!
bool
Track::analyze(bool validTracks[2][80])
{
    bool valid = true;

    if (side_m >= 2)
    {
        printf("invalid track sides_m: %d\n", side_m);
        return false;
    }

    if (track_m >= 80)
    {
        printf("invalid track track_m: %d\n", track_m);
        return false;
    }

    printf("Track %d\n", track_m);

    for (unsigned int i = 0; i < sectors_m.size(); ++i)
    {
        valid &= sectors_m[i]->analyze();
    }

    validTracks[side_m][track_m] = valid;

    return true;
}


//! writeToFile
//!
//! writes the track to a file in h17disk format
//!
//! @param file   ofstream for file
//!
//! @return  success
//!
bool
Track::writeToFile(std::ofstream &file)
{
    uint32_t size = 0;

    // determine each sector's size 
    for (uint16_t i = 0; i < sectors_m.size(); i++)
    {
        size += sectors_m[i]->getBlockSize();
    }

    // generate the header
    uint8_t buf[headerSize_c] = { 
                                    H17Disk::TrackDataId,
                                    side_m,
                                    track_m, 
                                    (uint8_t) ((size >> 8) & 0xff),
                                    (uint8_t) (size & 0xff)
                                };

    // write header
    file.write((const char*) buf, headerSize_c);

    // write each sector
    for (uint16_t i = 0; i < sectors_m.size(); i++)
    {
        sectors_m[i]->writeToFile(file);
    }

    return true;
}


//! write track in H8D format
//!
//! writes the track to a file
//!
//! @param file   ofstream for file
//!
//! @return  success
//!
bool
Track::writeH8D(std::ofstream &file)
{
    // determine each sector's size 
    for (uint8_t i = 0; i < 10; i++)
    {
        for(uint16_t j = 0; j < sectors_m.size(); j++)
        {
            if (i == sectors_m[j]->getSectorNum())
            {
                sectors_m[j]->writeToH8D(file);
            }
        }
    }

    return true;
}


//! write track in Raw format
//!
//! writes the track to a file
//!
//! @param file   ofstream for file
//!
//! @return  success
//!
bool
Track::writeRaw(std::ofstream &file)
{   

    for (uint8_t i = 0; i < 10; i++)
    {   
        for(uint16_t j = 0; j < sectors_m.size(); j++)
        {   
            if (i == sectors_m[j]->getSectorNum())
            {   
                sectors_m[j]->writeToRaw(file);
            }
        }
    }   
    
    return true;
}

