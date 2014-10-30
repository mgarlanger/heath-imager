/* Heathkit H17 Hard-sectored Disks */

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <usb.h>
#include <string.h>
#include "heath_hs.h"
#include "fc5025.h"
#include "decode.h"

HeathHSDisk::HeathHSDisk(BYTE sides,
			BYTE tracks,
			BYTE tpi,
			WORD maxSpeed): maxSide_m(sides),
				maxTrack_m(tracks),
				tpi_m(tpi),
				speed_m(maxSpeed)
{

}
// DSE:  above modified to include selection of RPM from GUI
// DSE:   required change to heath_hs.h and to heathimager.cpp in src/heathimager directory


HeathHSDisk::~HeathHSDisk()
{

}


BYTE HeathHSDisk::minTrack(void)
{
    return 0;
}

BYTE HeathHSDisk::maxTrack(void)
{
    return maxTrack_m;
}

//  DSE:  Added following two functions for speed control
WORD HeathHSDisk::minSpeed(void)
{
    return 300;
}

WORD HeathHSDisk::maxSpeed(void)
{
    return speed_m;
}
//  DSE:  #######################

BYTE HeathHSDisk::minSide(void)
{
    return 0;
}

BYTE HeathHSDisk::maxSide(void)
{
    return maxSide_m;
}

BYTE HeathHSDisk::minSector(BYTE track, BYTE side)
{
    return 1;
}

BYTE HeathHSDisk::maxSector(BYTE track, BYTE side)
{
    return 10;
}

BYTE HeathHSDisk::tpi(void)
{
    return tpi_m;
}

BYTE HeathHSDisk::density(void)
{
    // signle density is 1 for fc5025.
    return 1;
}

BYTE HeathHSDisk::physicalTrack(BYTE track)
{
    if (tpi_m == 96)
    {
        return track;
    }
    else
    {
        return track * 2;
    }
}

//  DSE:  Added following function to set speed from GUI
void HeathHSDisk::setSpeed(WORD rpm)
{
  if(rpm == 300)
  {
     RPMparam = 5555;
  } else {
     RPMparam = 6666;
  }
}

bool HeathHSDisk::setSides(BYTE sides)
{
    bool returnVal = true;

    if (sides == 2)
    {
        maxSide_m = 1;
    }
    else
    {
        maxSide_m = 0;
        returnVal = (sides == 1);
    }
    
    return returnVal;
}

bool HeathHSDisk::setTracks(BYTE tracks)
{
    bool returnVal = true;

    if (tracks == 80)
    {
        tpi_m = 96;
        maxTrack_m = 79;
    }
    else
    {
        tpi_m = 48;
        maxTrack_m = 39;
        returnVal = (tracks == 40);
    }
    
    return returnVal;
}

int HeathHSDisk::sectorBytes(BYTE side, BYTE track, BYTE sector)
{
    return sectorBytes_c; 
}

int HeathHSDisk::sectorRawBytes(BYTE side, BYTE track, BYTE sector)
{
    return sectorRawBytes_c;
}

int HeathHSDisk::readSector(BYTE *buffer, BYTE *rawBuffer, BYTE side, BYTE track, BYTE sector)
{
    int             xferlen = sectorRawBytes_c;
    int             xferlen_out;
    unsigned char   raw[sectorRawBytes_c];
    unsigned char   data[sectorBytes_c];
    struct
    {
        FC5025::Opcode  opcode;
        uint8_t         flags;
        FC5025::Format  format;
        uint16_t        bitcell;
        uint8_t         sectorhole;
        uint8_t         rdelayh;
        uint16_t        rdelayl;
        uint8_t         idam;
        uint8_t         id_pat[12];
        uint8_t         id_mask[12];
        uint8_t         dam[3];
    } __attribute__ ((__packed__)) cdb =
    {
        FC5025::Opcode::ReadFlexible,  // opcode
        side,                          // flags
        FC5025::Format::FM,            // format
        htons(RPMparam),                   // bitcell
        sector,                        // sectorhole
        0,                             // rdelayh
        0,                             // rdelayl - no delay, start instantly after the hole.
        0x0,                           // idam
        {0, },                         // id_pat ... idmask .. dam
    };
    int   status = No_Error;
    
    FC5025::inst()->bulkCDB(&cdb, sizeof(cdb), 4000, NULL, raw, xferlen, &xferlen_out);
    if (xferlen_out != xferlen)
    {
        printf("%s - xferlen: %d  xferlen_out: %d\n", __FUNCTION__, xferlen, xferlen_out);
        status = Err_ReadError;
        return status;
    }
    else
    {
        printf("%s - xfer success\n", __FUNCTION__);
    }

    if (Decode::decodeFM(data, raw, sectorBytes_c) != 0)
    {
        status = Err_InvalidClocksBits;
        if (rawBuffer)
        {
            memcpy(rawBuffer, raw, sectorRawBytes_c);
        }
        return status;
    }

    status = processSector(data, side, track, sector);

    printf("%s- processStatus; %d\n", __FUNCTION__, status);

    // Copy raw data back if buffer provided. 
    if (rawBuffer)
    { 
        memcpy(rawBuffer, raw, sectorRawBytes_c);
    }

    // Copy data back if buffer provided.
    if (buffer)
    {
        memcpy(buffer, data, sectorBytes_c);
    }

    return status;
}


int HeathHSDisk::trackBytes(BYTE head, BYTE track)
{
    return sectorBytes_c * 10;
}

int HeathHSDisk::trackRawBytes(BYTE head, BYTE track)
{
    return sectorRawBytes_c * 10;
}

BYTE HeathHSDisk::updateChecksum(BYTE checksum, BYTE val)
{
    // First XOR it.
    checksum ^= val;

    // Now rotate left
    checksum = (checksum >> 7) | (checksum << 1);

    return checksum;
}

int HeathHSDisk::alignSector(BYTE *out, BYTE *in)
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

    for (; pos < sectorBytes_c; pos++)
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

    for (; pos < sectorBytes_c; pos++)
    {
        
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

BYTE HeathHSDisk::reverseChar(BYTE val)
{
    BYTE newVal = 0;
    for (int i = 0; i < 8; i++)
    {
        newVal <<= 1;
        // remove if ->  newVal |= (val & 0x01);
        if (val & 0x01)
        {
            newVal |= 0x01;
        }
        val >>= 1;
        
    }
    return newVal;
}



BYTE HeathHSDisk::shiftByte(BYTE first,
                            BYTE second, 
                            BYTE shift)
{
    unsigned int val = (((unsigned int) first) << 8) |  second;

    val >>= (8-shift);
    val &= 0xff;

    return reverseChar(val);
}


int HeathHSDisk::processSector(BYTE *buffer, BYTE side, BYTE track, BYTE sector)
{
    // expect the sync (0xfd) character first.
    //
    BYTE out[sectorBytes_c];
    int pos   = 0;
    int error = 0;
  
    error = alignSector(out, buffer);

    if (error)
    {
        return error;
    }    
    BYTE checkSum;

    memcpy(buffer, out, sectorBytes_c);

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

