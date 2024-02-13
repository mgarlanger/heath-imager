//! \file disk_util.h
//!
//! Processing imaged data blocks
//!

#ifndef __DISK_UTILS_H__
#define __DISK_UTILS_H__

#include <stdint.h>

enum
{
    //! No error decoding the sector
    No_Error                  = 0,

    //! Error from the FC5025, did not provide data back
    Err_ReadError             = 1,

    //! Could not extract the data from the clocks
    Err_InvalidClocksBits     = 2,

    //! Header sync was missing
    Err_MissingHeaderSync     = 3,

    //! Track number in header does not match expected track
    Err_WrongTrack            = 4,

    //! sector is out of range 
    Err_InvalidSector         = 5,

    //! header checksum is not valid
    Err_InvalidHeaderChecksum = 6,

    //! data sync missing 
    Err_MissingDataSync       = 7,

    //! data checksum is not valid
    Err_InvalidDataChecksum   = 8
};

extern const char *sectorErrorStrings[];


//! expected sync character for Heath hard-sectored
const uint8_t         PrefixSyncChar_c = 0xfd;


//!
//!  Process the sector
//!  
//!  Steps
//!
//!  1) Align sector
//!  2) Find header sync
//!  3) copy out header
//!  4) Find data sync
//!  5) copy out data
//!  6) update and look for next sync
int  processSector(uint8_t *buffer,
                   uint8_t *out,
                   uint16_t length,
                   uint8_t  side,
                   uint8_t  track,
                   uint8_t  sector);


//!
//! Align sector based on sync bytes, aligns both the header and data blocks
//! and start of next header, if it finds it.
//!
//! @param out     pointer to output buffer aligned if sync bytes are pressed
//! @param in      pointer to input buffer
//! @param length  length of input buffer
int  alignSector(uint8_t  *out,
                 uint8_t  *in,
                 uint16_t  length,
                 uint8_t   syncByte = PrefixSyncChar_c);


int  alignSector2(uint8_t  *out,
                  uint8_t  *in,
                  uint16_t  length,
                  uint8_t   syncByte = PrefixSyncChar_c);

//!
//! Updates checksum from existing checksum and new character value
//!
//! @param checksum  existing checksum
//! @param val       new character to add to checksum
//!
//! \retval  new checksum
//!
uint8_t updateChecksum(uint8_t checksum,
                       uint8_t val);


//!
//! Reverse character bits.
//!
//!   Exchange bits
//!   0 <-> 7
//!   1 <-> 6
//!   2 <-> 5
//!   3 <-> 4
//!
//! @param  val character to reverse the bits
//!
//! \retval bits reversed
//!
uint8_t reverseChar(uint8_t val);


//!
//! Shift bytes to extract the next byte
//!
//! @param first   high byte
//! @param second  low byte
//! @param shift   number of bits to shift right
//!
//! \retval extracted byte
//!
//! \todo update the processing.
uint8_t shiftByte(uint8_t first,
                  uint8_t second,
                  uint8_t shift);


#endif
