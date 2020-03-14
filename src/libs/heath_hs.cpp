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
//! @param sides       number of sides for the disk
//! @param tracks      number of tracks for the disk
//! @param driveTpi    tpi of drive being used to image the disk
//! @param driveRpm    rpm of the drive being used to image the disk
//!
HeathHSDisk::HeathHSDisk(BYTE sides,
                         BYTE tracks,
                         BYTE driveTpi,
                         WORD driveRpm): maxSide_m(sides),
                                         maxTrack_m(tracks),
                                         driveRpm_m(driveRpm),
                                         driveTpi_m(driveTpi)
{
    // for 360 RPM drives - default
    if (driveRpm_m == 300) {
        printf("Using 300 RPMs\n");
        bitcellTiming_m = 8000;
    }
    else {
        printf("Using 360 RPMs\n");
        bitcellTiming_m = 6666;
    }
    if (tracks == 80)
    {
        tpi_m      = 96;
    }
    else
    {
        tpi_m      = 48;
    }

    printf("maxSides_m: %d, maxTrack_m: %d, driveRpm_m: %d, driveTpi_m: %d\n", maxSide_m, maxTrack_m, driveRpm_m, driveTpi_m);
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


//! return RPM - rotations per minute
//!
//! @return speed
//!
WORD
HeathHSDisk::driveRpm(void)
{
    return driveRpm_m;
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
    printf("physicalTrack, driveTpi: %d, tpi: %d\n", driveTpi_m, tpi_m);
 
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
            // 1 to 1 mapping
            return track;
        }
    }
}


//! set drive rpm
//!
//! @param rpm
//!
void
HeathHSDisk::setDriveRpm(WORD rpm)
{
     // if drive is 300 RPM, (not a TEAC 1.2M) then set bitcell timing to the slower speed
     if(rpm == 300)
     {
         bitcellTiming_m = 8000;
     } 
     else
     {
         // otherwise rpm == 360 default speed.
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
        // default to 1
        maxSide_m = 1;

        // flag error if side param invalid
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
        // default to 48 tpi
        tpi_m      = 48;
        maxTrack_m = 40;

        // flag error if param invalid
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
    unsigned char   raw[sectorRawBytes_c];  // as read in from the fc5025
    unsigned char   data[sectorBytes_c];    // after removing clock-bits
    unsigned char   out[sectorBytes_c];     // after processing sector for alignment

    int   status = No_Error;
   
    //printf("bit timing: %d\n", bitcellTiming_m);

    status = FC5025::inst()->readHardSectorSector(raw, sectorRawBytes_c, side, track, sector, bitcellTiming_m); 
    if (status) {
       printf("%s - readSector failed: %d\n", __FUNCTION__, status);
    }

    // printf("%s - xfer success\n", __FUNCTION__);
   
    if (rawBuffer)
    {
        memcpy(rawBuffer, raw, sectorRawBytes_c);
    }
 

    if (Decode::decodeFM(data, raw, sectorBytes_c) != 0)
    {
        status = Err_InvalidClocksBits;
        return status;
    }

    BYTE expectedTrackNum;

    if (maxSide_m == 2) {
        expectedTrackNum = (track << 1) + side;
    } 
    else
    {
        expectedTrackNum = track;
    }
 
    status = processSector(data, out, sectorBytes_c, side, expectedTrackNum, sector);

    printf("%s- processStatus: %d\n", __FUNCTION__, status);

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

#if 0
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
HeathHSDisk::processSector(BYTE *buffer, BYTE *out, WORD length, BYTE side, BYTE track, BYTE sector)
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
    BYTE checkSum;

    // copy aligned buffer from out back to buffer
    memcpy(buffer, out, length);

    // look for sync character
    for (pos = 0; pos < 57; pos++)
    {
        if (buffer[pos] == PrefixSyncChar_c)
        {
            break;
        }
    }

    if (buffer[pos] != PrefixSyncChar_c)
    {
        //printf("Header sync missed (T: %d S: %d)\n", track, sector);
        return Err_MissingHeaderSync;
    }
    //printf("Header: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buffer[pos], buffer[pos+1], buffer[pos+2], buffer[pos+3], buffer[pos+4], buffer[pos+5]);
    //checkSum = 0;
    checkSum = updateChecksum(       0, buffer[++pos]);  // Volume Number
    checkSum = updateChecksum(checkSum, buffer[++pos]);  // track Number

    // TODO update to handle track numbers of double-sided 80-track
    // Disks, track number appears to be in b7-b1, side is in b0

    if (buffer[pos] != track)
    {
        printf("**** Unexpected track - expected: %d  received: %d\n", track, buffer[pos]);
        //return Err_WrongTrack;
    }
   
    checkSum = updateChecksum(checkSum, buffer[++pos]);  // sector

    // Sector number must be between 0 and 9.
    if (buffer[pos] >= 10)
    {
        //printf("Invalid sector - expected: %d  received: %d\n", sector - 1, buffer[pos]);
        //issue = true; \todo just have to check that all 10 sectors are present and no duplicates. - can't expect this to equal.
        return Err_InvalidSector;
    }

    // update with the checksum read from the disk
    checkSum = updateChecksum(checkSum, buffer[++pos]); // checksum value

    // value should now be zero
    if (checkSum)
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

    if (buffer[pos] != PrefixSyncChar_c)
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
        checkSum = updateChecksum(checkSum, buffer[++pos]);
    }
/*
    // update checksum with checksum value read from disk
    checkSum = updateChecksum(checkSum, buffer[++pos]);

    // Should be zero if valid
    if (checkSum)
    {
        //printf("Invalid Data Checksum: %d\n", checkSum);
       return Err_InvalidDataChecksum;
    }
*/
    ++pos;
    if (checkSum != buffer[pos])
    {
       printf("Invalid Data Checksum: calc: 0x%02x, read: 0x%02x\n", checkSum, buffer[pos]);
       return Err_InvalidDataChecksum;
    }
    return No_Error;
}
#endif
