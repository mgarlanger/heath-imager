/* Heathkit H17 Hard-sectored Disks */

#include "heath_hs.h"
#include "fc5025.h"
#include "decode.h"
#include "disk_util.h"

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <usb.h>
#include <string.h>

HeathHSDisk::HeathHSDisk(BYTE sides,
                         BYTE tracks,
                         BYTE tpi,
                         WORD rpm): maxSide_m(sides),
                                 maxTrack_m(tracks),
                                 tpi_m(tpi),
                                 speed_m(rpm)
{
   bitcellTiming_m = 6666;
}

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

WORD HeathHSDisk::minSpeed(void)
{
    return 300;
}

WORD HeathHSDisk::rpm(void)
{
    return speed_m;
}


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
    return 0;
}

BYTE HeathHSDisk::maxSector(BYTE track, BYTE side)
{
    return 9;
}

BYTE HeathHSDisk::tpi(void)
{
    return tpi_m;
}

BYTE HeathHSDisk::density(void)
{
    // single density is 1 for fc5025.
    return 1;
}

BYTE HeathHSDisk::physicalTrack(BYTE track)
{
    // if the disk is 96 tpi, then it's a 1 to 1 mapping with the TEAC 1.2M
    if (tpi_m == 96)
    {
        return track;
    }
    else
    {
        // otherwise have to skip tracks
        return track * 2;
    }
}

void HeathHSDisk::setSpeed(WORD rpm)
{
  // if drive is 300 RPM, (not a TEAC 1.2M) then set bitcell timing to the slower speed
  if(rpm == 300)
  {
     bitcellTiming_m = 5555;
  } else {
     // otherwise default speed.
     bitcellTiming_m = 6666;
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
        tpi_m      = 96;
        maxTrack_m = 79;
    }
    else
    {
        tpi_m      = 48;
        maxTrack_m = 39;
        returnVal  = (tracks == 40);
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
    unsigned char   out[sectorBytes_c];
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
        htons(bitcellTiming_m),        // bitcell
        (uint8_t) (sector + 1),        // sectorhole - FC5025 expect one based number instead of zero based.
        0,                             // rdelayh
        0,                             // rdelayl - no delay, start instantly after the hole.
        0x0,                           // idam
        {0, },                         // id_pat ... idmask .. dam
    };
    int   status = No_Error;
   
    printf("bit timing: %d\n", bitcellTiming_m);
 
    FC5025::inst()->bulkCDB(&cdb, sizeof(cdb), 4000, NULL, raw, xferlen, &xferlen_out);
    if (xferlen_out != xferlen)
    {
        printf("%s - xferlen: %d  xferlen_out: %d\n", __FUNCTION__, xferlen, xferlen_out);
        status = Err_ReadError;
        return status;
    }
    
    printf("%s - xfer success\n", __FUNCTION__);
   
    if (rawBuffer)
    {
        memcpy(rawBuffer, raw, sectorRawBytes_c);
    }
 

    if (Decode::decodeFM(data, raw, sectorBytes_c) != 0)
    {
        status = Err_InvalidClocksBits;
        return status;
    }

    status = processSector(data, out, sectorBytes_c, side, track, sector);

    printf("%s- processStatus; %d\n", __FUNCTION__, status);

    // Copy data back if buffer provided.
    if (buffer)
    {
        memcpy(buffer, out, sectorBytes_c);
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

