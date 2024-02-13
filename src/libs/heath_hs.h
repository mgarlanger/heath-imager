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
    HeathHSDisk(uint8_t sides,
                uint8_t tracks,
                uint8_t driveTpi,
                uint16_t driveRpm);

    virtual ~HeathHSDisk();

    virtual uint8_t minTrack(void);
    virtual uint8_t maxTrack(void);

    virtual uint16_t driveRpm(void);

    virtual uint8_t minSide(void);
    virtual uint8_t maxSide(void);

    virtual void setDriveRpm(uint16_t speed);
    virtual void setDriveTpi(uint8_t tpi);

    virtual uint8_t minSector(uint8_t track,
                              uint8_t side);
    virtual uint8_t maxSector(uint8_t track,
                              uint8_t side);

    virtual uint8_t tpi(void);
    virtual uint8_t density(void);

    virtual uint16_t sectorBytes(uint8_t side,
                                 uint8_t track,
                                 uint8_t sector);
    virtual uint16_t sectorRawBytes(uint8_t side,
                                    uint8_t track,
                                    uint8_t sector);

    // \todo remove trackBytes, since it's not relevant anymore.
    virtual uint16_t trackBytes(uint8_t side,
                                uint8_t track);
    virtual uint16_t trackRawBytes(uint8_t side,
                                   uint8_t track);

    virtual uint8_t physicalTrack(uint8_t track);

    virtual int  readSector(uint8_t *buffer,
                            uint8_t *rawBuffer,
                            uint8_t  side,
                            uint8_t  track,
                            uint8_t  sector);
#if 0
    virtual int  processSector(uint8_t *buffer,
                               uint8_t *out,
                               uint16_t length,
                               uint8_t side,
                               uint8_t track,
                               uint8_t sector);
#endif
    virtual bool setSides(uint8_t    sides);
    virtual bool setTracks(uint8_t   tracks);

    static int   defaultSectorBytes()    { return sectorBytes_c; };
    static int   defaultSectorRawBytes() { return sectorRawBytes_c; };

private:

    uint8_t  maxSide_m;
    uint8_t  maxTrack_m;
    uint8_t  tpi_m;
    uint16_t driveRpm_m;
    uint8_t  driveTpi_m;
    uint16_t bitcellTiming_m;

    // ideal sector size is 320 bytes, based on Heath's manual of 62.5 microSecond
    // per character, friction and other things such at drive's actual RPM can affect that.
    // set it so it should overlap the next sector.
    static const uint16_t sectorBytes_c    = 350;

    // over read the sector - double the data bits due to clock bits.
    static const uint16_t sectorRawBytes_c = 700;
};

#endif
