#ifndef __DISK__
#define __DISK__

#include "hi_types.h"

struct SectorList
{
    unsigned int  sector;
//    unsigned int  offset;
//    unsigned int  rawOffset;
};

class Disk
{
public:

    enum
    {
        No_Error                  = 0,
        Err_ReadError             = 1,
        Err_InvalidClocksBits     = 2,
        Err_MissingHeaderSync     = 3,
        Err_WrongTrack            = 4,
        Err_InvalidSector         = 5,
        Err_InvalidHeaderChecksum = 6,
        Err_MissingDataSync       = 7,
        Err_InvalidDataChecksum   = 8
    };

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

    virtual BYTE minSector(BYTE track,
                           BYTE side) = 0;
    virtual BYTE maxSector(BYTE track,
                           BYTE side) = 0;
    virtual BYTE numSectors(BYTE track,
                            BYTE side);

    virtual BYTE tpi(void) = 0;
    virtual BYTE density(void) = 0;

    virtual int  sectorBytes(BYTE side,
                             BYTE track,
                             BYTE sector) = 0;
    virtual int  sectorRawBytes(BYTE side,
                                BYTE track,
                                BYTE sector) = 0;

    virtual int  trackBytes(BYTE side, 
                            BYTE track) = 0;
    virtual int  trackRawBytes(BYTE side,
                               BYTE track) = 0;

    virtual BYTE physicalTrack(BYTE track) = 0;

    virtual int readSector(BYTE *buffer,
                           BYTE *rawBuffer,
                           BYTE side,
                           BYTE track,
                           BYTE sector) = 0;

    virtual void genBestReadOrder(SectorList *out,
                                  int         track,
                                  int         side);

private:
    virtual void halfTheSectors(SectorList **out,
                                int track,
                                int side,
                                int which_half); 

};

#endif

