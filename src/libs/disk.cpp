//! \file disk.cpp
//!
//! Disk base class
#include "disk.h"

#include <stdlib.h>
#include <string.h>



//! return the number of sides for the disk
//!
//! @return number of sides
BYTE
Disk::numSides(void)
{
    return maxSide() - minSide() + 1;
}


//! return the number of tracks for the disk
//!
//! @return number of tracks
BYTE
Disk::numTracks(void)
{
    return maxTrack() - minTrack() + 1;
}


//! return the number of sectors
//!
//! @param track
//! @param side
//! 
//! @return number of sectors
BYTE
Disk::numSectors(BYTE track, BYTE side)
{
    return maxSector(track, side) - minSector(track, side) + 1;
}


//! return an array of half the sectors to read in
//!
//! @param out
//! @param track
//! @param side
//! @param which_half
//!
void
Disk::halfTheSectors(SectorList **out, BYTE track, BYTE side, BYTE start)
{
    BYTE first = minSector(track, side) + start;
    BYTE last  = maxSector(track, side);

    for (BYTE sector = first; sector <= last; sector += 2)
    {
        (*out)->sector = sector;
        (*out)++;
    }
}


//! get the best read order for the given track and side
//!
//! @param out
//! @param track
//! @param side
//!
void
Disk::genBestReadOrder(SectorList *out, BYTE track, BYTE side)
{
    // even 0, 2, 4, ...
    halfTheSectors(&out, track, side, 0);

    // odd 1, 3, 5, ...
    halfTheSectors(&out, track, side, 1);
}

