#include "disk_util.h"

#include <cstring>


BYTE updateChecksum(BYTE checksum, BYTE val)
{
    // First XOR it.
    checksum ^= val;

    // Now rotate left
    checksum = (checksum >> 7) | (checksum << 1);

    return checksum;
}

int alignSector(BYTE *out, BYTE *in, WORD length)
{
    bool          foundHeadSync = false;
    bool          foundDataSync = false;
    unsigned int  pos;
    unsigned char bitOffset;
    unsigned char bitOffset2;

    // first find the header, it most occur within the first 57
    // bytes, otherwise there won't be room for everything -
    // 320 - (256 + 2 + 5)
    /// \todo - change to just 320 and try to get more out even if 
    /// it is not complete.
    for (pos = 0; pos < 57; pos++)
    {
        for (bitOffset = 0; bitOffset < 8; bitOffset++)
        {
            unsigned val = shiftByte(in[pos], in[pos+1], bitOffset);

            //printf("in[%d] = 0x%02x  val = 0x%02x bo = %d\n", pos, in[pos], val, bitOffset);
            if (val == PrefixSyncChar_c)
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

        out[pos] = 0;
    }

    // Copy out the header
    for (int i = 0; i < 5; i++)
    {
        out[pos] = shiftByte(in[pos], in[pos+1], bitOffset);
        pos++;
    }

    // now find data block
    for ( ; (pos < 62);  pos++)
    {
        for(bitOffset = 0; bitOffset < 8; bitOffset++)
        {
            unsigned val = shiftByte(in[pos], in[pos+1], bitOffset);

            if (val == PrefixSyncChar_c)
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

        out[pos] = 0;
    }

    for (int i = 0; i < 258; i++)
    {
        out[pos] = shiftByte(in[pos], in[pos+1], bitOffset);
        pos++;
    }

    for (; pos < length; pos++)
    {

        out[pos] = shiftByte(in[pos], in[pos+1], bitOffset);
        pos++;
        for(bitOffset2 = 0; bitOffset2 < 8; bitOffset2++)
        {
            unsigned val = shiftByte(in[pos], in[pos+1], bitOffset2);

            if (val == PrefixSyncChar_c)
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

        out[pos] = shiftByte(in[pos], in[pos+1], bitOffset);
    }

    for (; pos < length; pos++)
    {    /// \todo fix this... one byte past end of buffer.
        /// \todo determine if we should write this data or just put 0s.
        out[pos] = shiftByte(in[pos], in[pos+1], bitOffset2);
        pos++;
    }


#if 0
    for ( ; pos < sectorBytes_c; pos++)
    {
        for(bitOffset = 0; bitOffset < 8; bitOffset++)
        {
            unsigned val = shiftByte(in[pos], in[pos+1], bitOffset);

            if (val == PrefixSyncChar_c)
            {
                foundNextSync = true;  // needed to get out of the pos loop
                break;
            }
        }
        if (foundNextSync)
        {
            break;
        }

        out[pos] = 0;
    }
    for ( ; pos < sectorBytes_c; pos++)
    {
        out[pos] = shiftByte(in[pos], in[pos+1], bitOffset);
        pos++;
    }
#endif
    return No_Error;
}

BYTE reverseChar(BYTE val)
{
    BYTE newVal = 0;
    for (int i = 0; i < 8; i++)
    {
        newVal <<= 1;
        newVal |= (val & 0x01);
        val >>= 1;
    }

    return newVal;
}



BYTE shiftByte(BYTE first,
                            BYTE second,
                            BYTE shift)
{
    unsigned int val = (((unsigned int) first) << 8) |  second;

    val >>= (8-shift);
    val &= 0xff;

    return reverseChar(val);
}


int processSector(BYTE *buffer, BYTE *out, WORD length, BYTE side, BYTE track, BYTE sector)
{
    // expect the sync (0xfd) character first.
    //
    //BYTE out[sectorBytes_c];
    int pos   = 0;
    int error = 0;

    error = alignSector(out, buffer, length);

    if (error)
    {
        return error;
    }
    BYTE checkSum;

    memcpy(buffer, out, length);

    for (int i = 0; i < 20; i++)
    {
        if (buffer[pos] == PrefixSyncChar_c)
        {
            break;
        }

        pos++;
    }

    if (buffer[pos] != PrefixSyncChar_c)
    {
        //printf("Header sync missed (T: %d S: %d)\n", track, sector);
        return Err_MissingHeaderSync;
    }
    //printf("Header: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buffer[pos], buffer[pos+1], buffer[pos+2], buffer[pos+3], buffer[pos+4], buffer[pos+5]);
    checkSum = 0;
    checkSum = updateChecksum(checkSum, buffer[++pos]);  // Volume Number
    checkSum = updateChecksum(checkSum, buffer[++pos]);  // track Number

    if (buffer[pos] != track)
    {
        //printf("Invalid track - expected: %d  received: %d\n", track, buffer[pos]);
        return Err_WrongTrack;
    }

    checkSum = updateChecksum(checkSum, buffer[++pos]);  // sector Number
    if (buffer[pos] != (sector-1))
    {
        ////printf("Invalid sector - expected: %d  received: %d\n", sector - 1, buffer[pos]);
        //issue = true; \todo just have to check that all 10 sectors are present and no duplicates. - can't expect this to equal.
       // return Err_InvalidSector;
    }
    checkSum = updateChecksum(checkSum, buffer[++pos]); // checksum value
    if (checkSum)
    {
        //printf("Invalid Header Checksum: %d\n", checkSum);
        return Err_InvalidHeaderChecksum;
    }

    for (int i = 0; i < 64; i++)
    {
        if (buffer[pos] == PrefixSyncChar_c)
        {
            break;
        }
        pos++;
    }

    if (buffer[pos] != PrefixSyncChar_c)
    {
        //printf("Data sync missed (T: %d S: %d) - pos: 0x%06x\n", track, sector, pos);
        return Err_MissingDataSync;
    }

    checkSum = 0;
    // check all the data
    for (int i = 0; i < 256; i++)
    {
        checkSum = updateChecksum(checkSum, buffer[++pos]); // checksum value
    }

    checkSum = updateChecksum(checkSum, buffer[++pos]); // checksum value

    if (checkSum)
    {
        //printf("Invalid Data Checksum: %d\n", checkSum);
       return Err_InvalidDataChecksum;
    }

    return No_Error;
}

