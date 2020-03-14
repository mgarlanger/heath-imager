//! \file heath_hs.h
//!
//! Handles the h17disk file format.
//!

#ifndef __HEATH_HS_H__
#define __HEATH_HS_H__

#include "disk.h"


class HeathHSDisk: virtual public Disk
{
public:
    HeathHSDisk(BYTE sides,
                BYTE tracks,
                BYTE driveTpi,
                WORD driveRpm);

    virtual ~HeathHSDisk();

    virtual BYTE minTrack(void);
    virtual BYTE maxTrack(void);

    virtual WORD driveRpm(void);

    virtual BYTE minSide(void);
    virtual BYTE maxSide(void);

    virtual void setDriveRpm(WORD speed);
    virtual void setDriveTpi(BYTE tpi);

    virtual BYTE minSector(BYTE track,
                           BYTE side);
    virtual BYTE maxSector(BYTE track,
                           BYTE side);

    virtual BYTE tpi(void);
    virtual BYTE density(void);

    virtual WORD sectorBytes(BYTE side,
                             BYTE track,
                             BYTE sector);
    virtual WORD sectorRawBytes(BYTE side,
                                BYTE track,
                                BYTE sector);

    // \todo remove trackBytes, since it's not relevant anymore.
    virtual WORD trackBytes(BYTE side,
                            BYTE track);
    virtual WORD trackRawBytes(BYTE side,
                               BYTE track);

    virtual BYTE physicalTrack(BYTE track);

    virtual int  readSector(BYTE *buffer,
                            BYTE *rawBuffer,
                            BYTE  side,
                            BYTE  track,
                            BYTE  sector);
#if 0
    virtual int  processSector(BYTE *buffer,
                               BYTE *out,
                               WORD length,
                               BYTE side,
                               BYTE track,
                               BYTE sector);
#endif
    virtual bool setSides(BYTE    sides);
    virtual bool setTracks(BYTE   tracks);

    static int   defaultSectorBytes()    { return sectorBytes_c; };
    static int   defaultSectorRawBytes() { return sectorRawBytes_c; };

private:

    BYTE maxSide_m;
    BYTE maxTrack_m;
    BYTE tpi_m;
    WORD driveRpm_m;
    BYTE driveTpi_m;
    WORD bitcellTiming_m;

    // ideal sector size is 320 bytes, based on Heath's manual of 62.5 microSecond
    // per character, friction and other things such at drive's actual RPM can affect that.
    // set it so it should overlap the next sector.
    static const WORD sectorBytes_c    = 350;

    // over read the sector - double the data bits due to clock bits.
    static const WORD sectorRawBytes_c = 700;
};

#endif

