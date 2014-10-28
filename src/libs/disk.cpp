// Disk base class
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <usb.h>

#include "disk.h"
#include "decode.h"
#include "fc5025.h"

BYTE
Disk::numSides(void)
{
    return maxSide() - minSide() + 1;
}

BYTE
Disk::numTracks(void)
{
    return maxTrack() - minTrack() + 1;
}

BYTE
Disk::numSectors(BYTE track, BYTE side)
{
    return maxSector(track, side) - minSector(track, side) + 1;
}

void
Disk::halfTheSectors(SectorList **out, int track, int side, int which_half)
{
    int             first = minSector(track, side);
    int             last = maxSector(track, side);
    int             sector;
//    int             offset = 0;
//    int             rawOffset = 0;

    for (sector = first; sector <= last; sector++)
    {
        if ((sector & 1) == which_half)
        {
            (*out)->sector = sector;
//            (*out)->offset = offset;
//            (*out)->rawOffset = rawOffset;
            (*out)++;
        }
//        offset += sectorBytes (track, side, sector);
//        rawOffset += sectorRawBytes (track, side, sector);
    }
}

void
Disk::genBestReadOrder(SectorList *out, int track, int side)
{
    // odd
    halfTheSectors(&out, track, side, 1);

    //even
    halfTheSectors(&out, track, side, 0);
}

