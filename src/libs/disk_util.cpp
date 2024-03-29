//!  \file disk_util.cpp
//!
//!  Processes disk related data blocks
//!

#include "disk_util.h"

#include <stdio.h>
#include <cstring>
#include <cctype> 
#include <stdint.h>

//! sector error codes converted to human text
//!
const char *sectorErrorStrings[] = {
    "No error.",
    "Read Error - no data",
    "Invalid clock bits.",
    "Missing Header sync byte.",
    "Wrong Track",
    "Invalid Sector",
    "Invalid header checksum",
    "Missing data sync byte",
    "Invalid data checksum",
};

//! Update checksum with new data byte
//!
//! @param checksum  - existing checksum
//! @param val       - data byte
//!
//!
//! @return updated checksum
uint8_t
updateChecksum(uint8_t checksum,
               uint8_t val)
{
    // First XOR it.
    checksum ^= val;

    // Now rotate left
    checksum = (checksum >> 7) | (checksum << 1);

    return checksum;
}


void
writeOutput(uint8_t  *out,
            uint16_t &pos,
            uint8_t   val,
            uint16_t  length)
{
    if (pos < length)
    {
        out[pos++] = val;
    }
}

//! Align sector data to the sync byte
//!
//! @param out      - buffer to store aligned data
//! @param in       - buffer of unaligned data
//! @param length   - size of buffer
//! @param syncByte - byte to sync buffer
//!
//! @return result
//!
int
alignSector(uint8_t  *out,
            uint8_t  *in,
            uint16_t  length,
            uint8_t   syncByte)
{
    bool          foundHeadSync = false;
    bool          foundDataSync = false;

    unsigned int  pos = 0;
    uint16_t      outPos = 0;
    unsigned char bitOffset = 0;
    unsigned char bitOffset2 = 0;

    for (int i = 0; i < 5; i++)
    {
        //out[outPos] = 0;
        writeOutput(out, outPos, 0, length);
    }

    // first find the header, it must occur within the first 87
    // bytes, otherwise there won't be room for everything -
    // 350 - (256 + 2 + 5)
    //! \todo - change to just 320 and try to get more out even if 
    //! it is not complete.
    for ( ; pos < 87; pos++)
    {
        for (bitOffset = 0; bitOffset < 8; bitOffset++)
        {
            unsigned val = shiftByte(in[pos], in[pos + 1], bitOffset);

            //printf("in[%d] = 0x%02x  val = 0x%02x bo = %d\n", pos, in[pos], val, bitOffset);
            if (val == syncByte)
            {
                foundHeadSync = true;  // needed to get out of the pos loop
                break;
            }
        }
        if (foundHeadSync)
        {
            //printf("foundHeadSync: pos - %d  bitOffset - %d\n",pos, bitOffset);
            break;
        }

        // either set everything to zero, or just copy out the in, or could even come back
        // and align this based on sync byte found
        // \todo determine which to do.
        //  out[pos] = 0;
        //  out[pos] = in[pos];
        //
        // Depends on what point you want the emulation to work on, if it's
        // on the bit level, then it should probably not be processed to
        // be aligned. The S2350 USART will be in sync mode and only present 
        // data once the pattern is found, so there is no harm in making
        // these zero
        //out[outPos++] = 0;
        writeOutput(out, outPos, 0, length);
    }

    // Copy out the header
    for (int i = 0; i < 5; i++)
    {
        //out[outPos++] = shiftByte(in[pos], in[pos + 1], bitOffset);
        writeOutput(out, outPos, shiftByte(in[pos], in[pos + 1], bitOffset), length);

        pos++;
    }

    // \todo should skip past the partial byte
    // out[pos++] = 0;

    // now find data block
    for ( ; pos < 92;  pos++)
    {
        for(bitOffset = 0; bitOffset < 8; bitOffset++)
        {
            uint8_t val = shiftByte(in[pos], in[pos + 1], bitOffset);

            if (val == syncByte)
            {
                foundDataSync = true;  // needed to get out of the pos loop
                break;
            }
        }
        if (foundDataSync)
        {
            //printf("foundDataSync: pos - %d  bitOffset - %d\n",pos, bitOffset);
            break;
        }

        // same as above.
        //out[outPos++] = 0;
        writeOutput(out, outPos, 0, length);
    }

    // copy data sector as aligned
    for (int i = 0; i < 258; i++)
    {
        //out[outPos++] = shiftByte(in[pos], in[pos + 1], bitOffset);
        writeOutput(out, outPos, shiftByte(in[pos], in[pos + 1], bitOffset), length);
        pos++;
    }

    // \todo should skip past the partial byte
    // out[pos++] = 0;

    // find next header sync, this will allow the full track to have the correct number of
    // bytes between sectors.
    foundHeadSync = false;
    for (; pos < length; pos++)
    {
        for(bitOffset2 = 0; bitOffset2 <= 8; bitOffset2++)
        {
            uint8_t val = shiftByte(in[pos], in[pos + 1], bitOffset2);

            if (val == syncByte)
            {
                // check to see if next byte is the sync, if so, add the zero
                // and set offset to 0
                if (bitOffset2 == 8) 
                {
                   //out[pos++] = 0;
                   // writeOutput(out, outPos, 0, bitOffset), length);
                   bitOffset2 = 0;
                }

                writeOutput(out, outPos, 0, length);

                foundHeadSync = true;  // needed to get out of the pos loop
                break;
            }
        }
        if (foundHeadSync)
        {
            //printf("foundDataSync: pos - %d  bitOffset - %d\n",pos, bitOffset);
            break;
        }
        // the end of the track is not where it would be
        // doing a sync, so until the next one, copy everything out
        // with current bit offset
        //out[pos] = shiftByte(in[pos], in[pos + 1], bitOffset);
        writeOutput(out, outPos, shiftByte(in[pos], in[pos + 1], bitOffset), length);
    }

    for (; pos < (unsigned int) length - 1; pos++)
    {
        //! \todo determine if we should write this data or just put 0s.
        //out[pos] = shiftByte(in[pos], in[pos + 1], bitOffset2);
        writeOutput(out, outPos, shiftByte(in[pos], in[pos + 1], bitOffset2), length);
    }

    if (pos == (unsigned int) (length - 1) ) {
        //out[pos] = shiftByte(in[pos], 0, bitOffset2);
        writeOutput(out, outPos, shiftByte(in[pos], 0, bitOffset2), length);
    }

    while (outPos < length)
    {
        writeOutput(out, outPos, 0, length);
    }

    return No_Error;
}

//! Pre-calculated array to reverse all the bits in a byte.
//
static const uint8_t BitReverseTable[] = 
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

//! reverse the bits in a byte
//!
//! @param val - value to reverse.
//!
//! @return the reversed byte.
//!
uint8_t
reverseChar(uint8_t val)
{
    return BitReverseTable[val];    
}


//! return a byte from 2 original bytes, shifted by the shift value
//! and reverse bits
//!
//! @param first  - first data byte
//! @param second - second data byte
//! @param shift  - shift value
//!
//! @return shift byte
//!
uint8_t
shiftByte(uint8_t first,
          uint8_t second,
          uint8_t shift)
{
    unsigned int val = (((unsigned int) first) << 8) |  second;

    val >>= (8-shift);
    val &= 0xff;

    return reverseChar(val);
}


int
validateHeader(uint8_t  *buffer,
               uint16_t  startPos)
{
    uint8_t checkSum = 0;

    uint8_t volume = buffer[startPos];
    uint8_t track = buffer[startPos + 1];
    uint8_t sector = buffer[startPos + 2];

    checkSum = updateChecksum(checkSum, volume);
    checkSum = updateChecksum(checkSum, track);
    checkSum = updateChecksum(checkSum, sector);

    return (checkSum == buffer[startPos + 3]);
}

//! process a sector to by aligning based on sync bytes, and reversal of the bits in each byte
//!
//! @param buffer - original data
//! @param out    - processed sector
//! @param length - length of buffer
//! @param side   - side sector was imaged from
//! @param track  - track of sector
//! @param sector - sector number
//!
//! @return result status
//!
int
processSector(uint8_t  *buffer,
              uint8_t  *out,
              uint16_t  length,
              uint8_t   side,
              uint8_t   track,
              uint8_t   sector)
{
    // expect the sync (0xfd) character first.
    //
    int pos   = 0;
    int error = 0;

    // align buffer and store it in out.
    error = alignSector(out, buffer, length);

    if (error)
    {
        return error;
    }

    uint8_t checkSum;

    // copy aligned buffer from out back to buffer
    memcpy(buffer, out, length);

    // look for sync character
    for (pos = 5; pos < 57; pos++)
    {
        if (buffer[pos] == PrefixSyncChar_c)
        {
            break;
        }
    }

    if (buffer[pos++] != PrefixSyncChar_c)
    {
        //printf("Header sync missed (T: %d S: %d)\n", track, sector);
        return Err_MissingHeaderSync;
    }
    uint8_t volumeRead   = buffer[pos++];
    uint8_t trackRead    = buffer[pos++];
    uint8_t sectorRead   = buffer[pos++];
    uint8_t checksumRead = buffer[pos++];

    //printf("Header: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buffer[pos], buffer[pos+1], buffer[pos+2], buffer[pos+3], buffer[pos+4], buffer[pos+5]);
    //checkSum = 0;
    checkSum = updateChecksum(       0, volumeRead);  // Volume Number
    checkSum = updateChecksum(checkSum, trackRead);   // track Number

    // TODO update to handle track numbers of double-sided 80-track
    // Disks, track number appears to be in b7-b1, side is in b0
    if (trackRead != track)
    {
        printf("**** Unexpected track - expected: %d  received: %d\n", track, trackRead);
        return Err_WrongTrack;
    }
    
    checkSum = updateChecksum(checkSum, sectorRead);  // sector

    // Sector number must be between 0 and 9.
    if (sectorRead >= 10)
    {
        //printf("Invalid sector - expected: %d  received: %d\n", sector - 1, sectorRead);
        //issue = true; \todo just have to check that all 10 sectors are present and no duplicates. - can't expect this to equal.
        return Err_InvalidSector;
    }

    // update with the checksum read from the disk
    //checkSum = updateChecksum(checkSum, buffer[++pos]); // checksum value

    // value should now be zero
    if (checkSum != checksumRead)
    {
        //printf("Invalid Header Checksum: %d\n", checkSum);
        return Err_InvalidHeaderChecksum;
    }

    // look for the sync prefix for the data portion of the sector
    for (int i = 0; i < 64; i++)
    {
        if (buffer[pos] == PrefixSyncChar_c)
        {
            break;
        }
        pos++;
    }

    if (buffer[pos++] != PrefixSyncChar_c)
    {
        //printf("Data sync missed (T: %d S: %d) - pos: 0x%06x\n", track, sector, pos);
        return Err_MissingDataSync;
    }

    // verify checksum of the data block

    // reset checksum value
    checkSum = 0;

    // check all the data
    for (int i = 0; i < 256; i++)
    {
        checkSum = updateChecksum(checkSum, buffer[pos++]);
    }

    if (checkSum != buffer[pos])
    {
       printf("Invalid Data Checksum: calc: 0x%02x, read: 0x%02x\n", checkSum, buffer[pos]);
       return Err_InvalidDataChecksum;
    }

    return No_Error;
}
