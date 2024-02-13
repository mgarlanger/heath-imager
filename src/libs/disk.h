//! \file disk.h
//!
//! Defines disk parameters.

#ifndef __DISK_H__
#define __DISK_H__

#include <stdint.h>

struct SectorList
{
    uint8_t  sector;
};


class Disk
{
public:

    enum Encoding
    {
        enc_FM,
        enc_MFM
    };

    virtual uint8_t minTrack(void) = 0;
    virtual uint8_t maxTrack(void) = 0;
    virtual uint8_t numTracks(void);

    virtual uint8_t minSide(void) = 0;
    virtual uint8_t maxSide(void) = 0;
    virtual uint8_t numSides(void);

    // sectors for a given track/side on heath's hard-sectored disks, it's always 10.
    virtual uint8_t minSector(uint8_t track,
                              uint8_t side) = 0;
    virtual uint8_t maxSector(uint8_t track,
                              uint8_t side) = 0;
    virtual uint8_t numSectors(uint8_t track,
                               uint8_t side);

    virtual uint8_t tpi(void) = 0;
    virtual uint8_t density(void) = 0;

    // number of bytes for the sector and the raw sector
    virtual uint16_t sectorBytes(uint8_t side,
                                 uint8_t track,
                                 uint8_t sector) = 0;
    virtual uint16_t sectorRawBytes(uint8_t side,
                                    uint8_t track,
                                    uint8_t sector) = 0;

    virtual uint16_t trackBytes(uint8_t side, 
                                uint8_t track) = 0;
    virtual uint16_t trackRawBytes(uint8_t side,
                                   uint8_t track) = 0;

    // for a given track number, the physical track on the 1.2M floppy
    virtual uint8_t physicalTrack(uint8_t track) = 0;

    virtual int readSector(uint8_t *buffer,
                           uint8_t *rawBuffer,
                           uint8_t side,
                           uint8_t track,
                           uint8_t sector) = 0;

    virtual void genBestReadOrder(SectorList *out,
                                  uint8_t        track,
                                  uint8_t        side);

private:
    virtual void halfTheSectors(SectorList **out,
                                uint8_t         track,
                                uint8_t         side,
                                uint8_t         which_half); 

};

#endif
