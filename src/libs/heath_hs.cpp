//! \file heath_hs.cpp
//!
//! Heathkit H17 Hard-sectored Disk h17disk file formats
//!

#include "heath_hs.h"
#include "decode.h"
#include "disk_util.h"
#include "fc5025.h"

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>


//! constructor
//!
//! @param sides
//! @param tracks
//! @param tpi
//! @param rpm
//!
HeathHSDisk::HeathHSDisk(BYTE sides,
                         BYTE tracks,
                         BYTE tpi,
                         WORD rpm): maxSide_m(sides),
                                    maxTrack_m(tracks),
                                    tpi_m(tpi),
                                    speed_m(rpm)
{
    // for 360 RPM drives - default
    bitcellTiming_m = 6667;
    // for 300 RPM drives
    //bitcellTiming_m = 8000;
}


//! destructor
//!
HeathHSDisk::~HeathHSDisk()
{

}


//! return minimum track number
//!
//! @return track number
//!
BYTE
HeathHSDisk::minTrack(void)
{
    return 0;
}


//! return maximum track number
//!
//! @return track number
//!
BYTE
HeathHSDisk::maxTrack(void)
{
    return maxTrack_m - 1;
}


//! return minimum speed
//!
//! @return speed
//!
WORD
HeathHSDisk::minSpeed(void)
{
    return 300;
}


//! return RPM - rotations per minute
//!
//! @return speed
//!
WORD
HeathHSDisk::rpm(void)
{
    return speed_m;
}


//! return minimum side number
//!
//! @return side number
//!
BYTE
HeathHSDisk::minSide(void)
{
    return 0;
}


//! return maximum side number
//!
//! @return side number
//!
BYTE
HeathHSDisk::maxSide(void)
{
    return maxSide_m - 1;
}


//! return minimum sector number
//!
//! @return sector number
//!
BYTE
HeathHSDisk::minSector(BYTE track, BYTE side)
{
    return 0;
}


//! return maximum sector number
//!
//! @return sector number
//!
BYTE
HeathHSDisk::maxSector(BYTE track, BYTE side)
{
    return 9;
}


//! return TPI - tracks per inch
//!
//! @return tpi
//!
BYTE
HeathHSDisk::tpi(void)
{
    return tpi_m;
}


//! return density
//!
//! @return density
//!
BYTE
HeathHSDisk::density(void)
{
    // single density is 1 for fc5025.
    return 1;
}


//! return physical track number to seek to based on type of disk
//! \todo handle 40 track drives
//!
//! @return physical track number
//!
BYTE
HeathHSDisk::physicalTrack(BYTE track)
{
    // if the disk is 96 tpi, then it's a 1 to 1 mapping with the TEAC 1.2M
    if (tpi_m == 96)
    {
        if (driveTpi_m == 96)
        {
            return track;
        }
        else
        {
            // \todo - unsupported, 96tpi disk in a 48 tpi drive.
            // exception 
            return track;
        }
    }
    else
    {
        // disk is 48 tpi
        if (driveTpi_m == 96)
        {
            // otherwise have to skip tracks
            return track * 2;
        }
        else 
        {
            return track;
        }
    }
}


//! set drive rpm
//!
//! @param rpm
//!
void
HeathHSDisk::setSpeed(WORD rpm)
{
     // if drive is 300 RPM, (not a TEAC 1.2M) then set bitcell timing to the slower speed
     if(rpm == 300)
     {
         bitcellTiming_m = 8000;
     } 
     else
     {
         // otherwise default speed.
         bitcellTiming_m = 6667;
     }
}


//! set drive tpi
//!
//! @param tpi
//!
void
HeathHSDisk::setDriveTpi(BYTE tpi)
{

     driveTpi_m = tpi;
}



//! set number of sides for the disk
//!
//! @param sides
//!
bool
HeathHSDisk::setSides(BYTE sides)
{
    bool returnVal = true;

    if (sides == 2)
    {
        maxSide_m = 2;
    }
    else
    {
        maxSide_m = 1;
        returnVal = (sides == 1);
    }
    
    return returnVal;
}


//! set the number of tracks for the disk
//!
//! @param tracks
//!
bool
HeathHSDisk::setTracks(BYTE tracks)
{
    bool returnVal = true;

    if (tracks == 80)
    {
        tpi_m      = 96;
        maxTrack_m = 80;
    }
    else
    {
        tpi_m      = 48;
        maxTrack_m = 40;
        returnVal  = (tracks == 40);
    }
    
    return returnVal;
}


//! return the number of bytes per sector
//!
//! @param side
//! @param track
//! @param sector
//!
//! @return number of bytes
//!
WORD
HeathHSDisk::sectorBytes(BYTE side, BYTE track, BYTE sector)
{
    return sectorBytes_c; 
}


//! return the number of bytes per raw sector
//!
//! @param side
//! @param track
//! @param sector
//!
//! @return number of bytes
//!
WORD
HeathHSDisk::sectorRawBytes(BYTE side, BYTE track, BYTE sector)
{
    return sectorRawBytes_c;
}


//! read a given sector of a disk through the FC5025 device
//!
//! @param buffer     buffer to write the processed sector
//! @param rawBuffer  buffer to write the raw sector
//! @param side       disk side to read
//! @param track      track to read
//! @param sector     sector to read 
//!
//! @return status
//!
int
HeathHSDisk::readSector(BYTE *buffer, BYTE *rawBuffer, BYTE side, BYTE track, BYTE sector)
{
    int             xferlen = sectorRawBytes_c;
    int             xferlen_out;
    unsigned char   raw[sectorRawBytes_c];  // as read in from the fc5025
    unsigned char   data[sectorBytes_c];    // after removing clock-bits
    unsigned char   out[sectorBytes_c];     // after processing sector for alignment

    struct
    {
        FC5025::Opcode  opcode;       // command for the fc5025
        uint8_t         flags;        // side flag
        FC5025::Format  format;       // density - FM/MFM
        uint16_t        bitcell;      // timing for a bit cell
        uint8_t         sectorhole;   // which physical sector to read
        uint8_t         rdelay_hi;    // delay after hole before starting to read
        uint16_t        rdelay_lo;
        uint8_t         idam;         // not used
        uint8_t         id_pat[12];   // not used
        uint8_t         id_mask[12];  // not used
        uint8_t         dam[3];       // not used
    } __attribute__ ((__packed__)) cdb =
    {
        FC5025::Opcode::ReadFlexible,  // opcode
        side,                          // flags
        FC5025::Format::FM,            // format
        htons(bitcellTiming_m),        // bitcell
        (uint8_t) (sector + 1),        // sectorhole - FC5025 expect one based number instead of zero based.
        0,                             // rdelay_hi - no delay, start immediately
        0,                             // rdelay_lo
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


//! returns the number of bytes for a track
//! 
//! @param head
//! @param track
//!
//! @return number of bytes
//!
WORD
HeathHSDisk::trackBytes(BYTE head, BYTE track)
{
    return sectorBytes_c * 10;
}


//! returns the number of byte for a raw track
//! 
//! @param head
//! @param track
//!
//! @return number of bytes
//!
WORD
HeathHSDisk::trackRawBytes(BYTE head, BYTE track)
{
    return sectorRawBytes_c * 10;
}

