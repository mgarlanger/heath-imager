#ifndef __DISK_UTILS_H__
#define __DISK_UTILS_H__


#include "hi_types.h"

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


const BYTE         PrefixSyncChar_c = 0xfd;

int  processSector(BYTE *buffer,
                   BYTE *out,
                   WORD length,
                   BYTE side,
                   BYTE track,
                   BYTE sector);

int  alignSector(BYTE  *out,
                 BYTE  *in,
                 WORD  length);

BYTE updateChecksum(BYTE checksum,
                    BYTE val);

BYTE reverseChar(BYTE val);

BYTE shiftByte(BYTE first,
               BYTE second,
               BYTE shift);



#endif
