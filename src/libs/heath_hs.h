#ifndef __HEATH_H__
#define __HEATH_H__

#include "disk.h"

class HeathHSDisk: virtual public Disk
{
public:
    HeathHSDisk(BYTE sides,
                BYTE tracks,
                BYTE tpi,
		WORD rpm);

    virtual ~HeathHSDisk();

    virtual BYTE minTrack(void);
    virtual BYTE maxTrack(void);

    virtual WORD rpm(void);
    virtual WORD minSpeed(void);

    virtual BYTE minSide(void);
    virtual BYTE maxSide(void);

    virtual void setSpeed(WORD speed);

    virtual BYTE minSector(BYTE track,
                           BYTE side);
    virtual BYTE maxSector(BYTE track,
                           BYTE side);

    virtual BYTE tpi(void);
    virtual BYTE density(void);

    virtual int  sectorBytes(BYTE side,
                             BYTE track,
                             BYTE sector);
    virtual int  sectorRawBytes(BYTE side,
                                BYTE track,
                                BYTE sector);

    virtual int  trackBytes(BYTE side,
                            BYTE track);
    virtual int  trackRawBytes(BYTE side,
                               BYTE track);

    virtual BYTE physicalTrack(BYTE track);

    virtual int  readSector(BYTE *buffer,
                            BYTE *rawBuffer,
                            BYTE side,
                            BYTE track,
                            BYTE sector);

    virtual bool setSides(BYTE    sides);
    virtual bool setTracks(BYTE   tracks);

    static int   defaultSectorBytes()    { return sectorBytes_c; };
    static int   defaultSectorRawBytes() { return sectorRawBytes_c; };

private:

    BYTE maxSide_m;
    BYTE maxTrack_m;
    BYTE tpi_m;
    WORD speed_m;
    WORD bitcellTiming_m;

    static const unsigned int sectorBytes_c    = 320;
    static const unsigned int sectorRawBytes_c = sectorBytes_c * 2;
    static const BYTE         PrefixSyncChar_c = 0xfd;
    
    virtual int  processSector(BYTE *buffer,
                               BYTE side,
                               BYTE track,
                               BYTE sector); 
    virtual int  alignSector(BYTE  *out,
                             BYTE  *in);

    virtual BYTE updateChecksum(BYTE checksum,
                                BYTE val);

    virtual BYTE reverseChar(BYTE val);

    virtual BYTE shiftByte(BYTE first,
                           BYTE second,
                           BYTE shift);
};

#endif

