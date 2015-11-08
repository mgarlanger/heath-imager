//! \file disk.h
//!
//! Defines disk parameters.

#ifndef __DISK__
#define __DISK__

#include "hi_types.h"


struct SectorList
{
    BYTE  sector;
};


class Disk
{
public:

    enum Encoding
    {
        enc_FM,
        enc_MFM
    };

    virtual BYTE minTrack(void) = 0;
    virtual BYTE maxTrack(void) = 0;
    virtual BYTE numTracks(void);

    virtual BYTE minSide(void) = 0;
    virtual BYTE maxSide(void) = 0;
    virtual BYTE numSides(void);

    // sectors for a given track/side on heath's hard-sectored disks, it's always 10.
    virtual BYTE minSector(BYTE track,
                           BYTE side) = 0;
    virtual BYTE maxSector(BYTE track,
                           BYTE side) = 0;
    virtual BYTE numSectors(BYTE track,
                            BYTE side);

    virtual BYTE tpi(void) = 0;
    virtual BYTE density(void) = 0;

    // number of bytes for the sector and the raw sector
    virtual WORD sectorBytes(BYTE side,
                             BYTE track,
                             BYTE sector) = 0;
    virtual WORD sectorRawBytes(BYTE side,
                                BYTE track,
                                BYTE sector) = 0;

    virtual WORD trackBytes(BYTE side, 
                            BYTE track) = 0;
    virtual WORD trackRawBytes(BYTE side,
                               BYTE track) = 0;

    // for a given track number, the physical track on the 1.2M floppy
    virtual BYTE physicalTrack(BYTE track) = 0;

    virtual int readSector(BYTE *buffer,
                           BYTE *rawBuffer,
                           BYTE side,
                           BYTE track,
                           BYTE sector) = 0;

    virtual void genBestReadOrder(SectorList *out,
                                  BYTE        track,
                                  BYTE        side);

private:
    virtual void halfTheSectors(SectorList **out,
                                BYTE         track,
                                BYTE         side,
                                BYTE         which_half); 

};

#endif

