//! \file h17disk.cpp
//!
//! Handles the h17disk file format.
//!

#include "h17disk.h"
#include "disk_util.h"
#include "h17block.h"
#include "decode.h"
#include "sector.h"
#include "raw_sector.h"
#include "raw_track.h"
#include "dump.h"


using namespace std;

const uint8_t H17Disk::DiskFormatBlock_c = 0x00;
const uint8_t H17Disk::FlagsBlock_c      = 0x01;
const uint8_t H17Disk::LabelBlock_c      = 0x02;
const uint8_t H17Disk::CommentBlock_c    = 0x03;
const uint8_t H17Disk::DateBlock_c       = 0x04;
const uint8_t H17Disk::ImagerBlock_c     = 0x05;
const uint8_t H17Disk::ProgramBlock_c    = 0x06;

const uint8_t H17Disk::DataBlock_c       = 0x10;
const uint8_t H17Disk::RawDataBlock_c    = 0x30;

// SubBlock IDs
//
const uint8_t H17Disk::TrackDataId       = 0x11;
const uint8_t H17Disk::SectorDataId      = 0x12;

const uint8_t H17Disk::RawTrackDataId    = 0x31;
const uint8_t H17Disk::RawSectorDataId   = 0x32;

// flags
const uint8_t H17Disk::DistUnknown            = 0x00;
const uint8_t H17Disk::DistributionDisk       = 0x01;
const uint8_t H17Disk::WorkingDisk            = 0x02;
const uint8_t H17Disk::CopyOfDistributionDisk = 0x03;

//
const uint8_t H17Disk::TrackDataUnknown                    = 0x00;
const uint8_t H17Disk::TrackDataGeneratedFromH8dConversion = 0x00;
const uint8_t H17Disk::TrackDataCreatedOnEmulator          = 0x01;
const uint8_t H17Disk::TrackDataCapturedOnH89              = 0x02;
const uint8_t H17Disk::TrackDataCapturedOnFC5025           = 0x03;

const uint8_t H17Disk::versionMajor_c = 1;
const uint8_t H17Disk::versionMinor_c = 0;
const uint8_t H17Disk::versionPoint_c = 0;

const uint8_t H17Disk::MandatoryFlagMask = 0x80;
const uint8_t H17Disk::MandatoryFlag_Mandatory = 0x80;
const uint8_t H17Disk::MandatoryFlag_NotMandatory = 0x00;

//!  Constructor
//!
//!  Defaults to single-sided, 40 tracks
//!
H17Disk::H17Disk(): sides_m(defaultSides_c),
                    tracks_m(defaultTracks_c),
                    curSide_m(0),
                    curTrack_m(0),
                    distribution_m(0),
                    trackDataSource_m(2),
                    writeProtect_m(false),
                    disableRaw_m(false),
                    versionMajor_m(versionMajor_c),
                    versionMinor_m(versionMinor_c),
                    versionPoint_m(versionPoint_c),
                    summarize_m(false),
                    sectorErrs_m(0)
{
    int i;
    for( i = 0; i < 10; i++)
    {
        curTrackSectors_m[i] = nullptr;
    }

    for( i = 0; i < 256; i++)
    {
        blocks_m[i] = nullptr;
    }


    //rawTracks_m.reserve(160);
}


//! destructor
//!
H17Disk::~H17Disk()
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    for(int  i = 0; i < 256; i++)
    {
        if (blocks_m[i])
        {
            delete blocks_m[i];
            blocks_m[i] = 0;
        }
    }

}

//! disable raw blocks
//!
void
H17Disk::disableRaw()
{
    disableRaw_m = true;
}

bool
H17Disk::fileExists(const char *name)
{
    if (FILE *file = fopen(name, "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }

}

//! open file for writing
//!
//! @param name       file name
//!
//! @return success
//!
bool
H17Disk::openForWrite(const char *name)
{

    if (fileExists(name))
    {
        return false;
    }
    // make sure a file is not already open
    if (file_m.is_open())
    {
        file_m.close();
    }

    file_m.open(name, ios::out | ios::binary);

    return (file_m.is_open());
}


//! open file for read
//!
//! @param name       file name
//!
//! @return success
//!
bool
H17Disk::openForRead(const char *name)
{
    // make sure file is not already open
    if (inFile_m.is_open())
    {
        inFile_m.close();
    }

    inFile_m.open(name, ios::in | ios::binary);

    return (inFile_m.is_open());
}


/*
//! open for recovery
//!
//! @param name       file name
//!
//! @return success
//!
bool
H17Disk::openForRecovery(const char *name)
{
    bool           status = false;

    if (!openForRead(name))
    {
        return status;
    }

    loadFile();
    return true;
}
*/


//! load file after inFile_m has been opened
//!
//! @return success
//!
bool
H17Disk::loadFile(const char *name)
{
    unsigned char *inputBuffer;
    bool           status = false;


    if (!openForRead(name))
    {
        return status;
    }

    inFile_m.seekg(0, std::ios::end);
    std::streamsize size = inFile_m.tellg();
    inFile_m.seekg(0, std::ios::beg);

    inputBuffer = new unsigned char[size];

    setDefaults();

    if ((inputBuffer) && (inFile_m.read((char *) inputBuffer, size)))
    {
        status = loadBuffer(inputBuffer, size);
    }

    delete[] inputBuffer;
    inFile_m.close();

    return status;
}


//! reprocess file
//!
//!  For all sectors with an error
//!    1) pick processed raw block with highest error number (lowest type of error).
//!    2) process header and data portions separately
//!
//! @return success
bool
H17Disk::reprocessFile()
{

    return true;
}


//! save file
//!
//! @param name file name
//!
//! @return success
//!
bool
H17Disk::saveFile(const char *name)
{
    printf("Attempting to write file: %s\n", name);
    if (!openForWrite(name))
    {
        printf("Unable to open file to write: %s\n", name);
        return false;
    }

    writeHeader();

    for (int i = 0; i < 256; i++)
    {
        if (blocks_m[i])
        {
            printf("Writing block: %d\n", i);
            blocks_m[i]->writeToFile(file_m);
        }
    }
    file_m.close();

    return true;
}


//! save h17disk image as a H8D file
//!
//! @param name      H8D file name
//!
//! @return success
//!
bool
H17Disk::saveAsH8D(const char *name)
{
    bool status = true;

    if (!openForWrite(name))
    {
        return false;

    }

    if (blocks_m[DataBlock_c])
    {
        status = ((H17DataBlock *) blocks_m[DataBlock_c])->writeAsH8D(file_m);
    }

    file_m.close();

    return status;
}


//! save h17disk image as a raw file - full track data including sector headers and sync bytes
//!
//! @param name      raw file name
//!
//! @return success
//!
bool
H17Disk::saveAsRaw(const char *name)
{
    bool status = true;

    if (!openForWrite(name))
    {
        return false;

    }

    if (blocks_m[DataBlock_c])
    {
        status = ((H17DataBlock *) blocks_m[DataBlock_c])->writeAsRaw(file_m);
    }

    file_m.close();

    return status;
}


//! analyze the disk image
//!
//! @return success
//!
bool
H17Disk::analyze()
{
    std::vector<uint8_t> optionalBlocks =  { DiskFormatBlock_c, FlagsBlock_c, LabelBlock_c,
                                             CommentBlock_c, DateBlock_c, ImagerBlock_c,
                                             ProgramBlock_c, RawDataBlock_c };

    if (blocks_m[DataBlock_c])
    {
        blocks_m[DataBlock_c]->analyze();
    }
    else
    {
        printf("Data block: %02x missing\n", DataBlock_c);
    }

    for (std::vector<uint8_t>::iterator it = optionalBlocks.begin(); it != optionalBlocks.end(); ++it)
    {
        if (blocks_m[*it])
        {
            blocks_m[*it]->analyze();
        }
        else
        {
            // printf("Optional block: %02x not included\n", *it);
        }
    }

    return true;
}


//! decode file and dump results to screen
//!
//! @param name       file name
//!
//! @return success
//!
bool
H17Disk::decodeFile(const char *name,
                    bool        summary)
{
    unsigned char *inputBuffer;
    bool           status = false;

    summarize_m = summary;
    if (!openForRead(name))
    {
        return status;
    }

    inFile_m.seekg(0, std::ios::end);
    std::streamsize size = inFile_m.tellg();
    inFile_m.seekg(0, std::ios::beg);

    inputBuffer = new unsigned char[size];

    if ((inputBuffer) && (inFile_m.read((char *) inputBuffer, size)))
    {
    //    status = decodeBuffer(inputBuffer, size);
    }

    delete[] inputBuffer;
    inFile_m.close();

    return status;
}

bool
H17Disk::dumpFileInfo(const char *name,
                      int         level)
{
    // unsigned char *inputBuffer;
    bool           status = false;

    if (!loadFile(name))
    {
        return status;
    }

    /*
    inFile_m.seekg(0, std::ios::end);
    std::streamsize size = inFile_m.tellg();
    inFile_m.seekg(0, std::ios::beg);

    inputBuffer = new unsigned char[size];

    if ((inputBuffer) && (inFile_m.read((char *) inputBuffer, size)))
    {
        status = dumpBuffer(inputBuffer, size, level);
    }
    delete[] inputBuffer; */

    for (int i = 0; i < 256; i++)
    {
        if (blocks_m[i])
        {
            blocks_m[i]->dump(level);
        }
    }

    inFile_m.close();

    return status;
}


//! decodes memory buffer of a h17disk file
//!
//! @param  buf
//! @param  size
//!
//! @return success
//!
bool
H17Disk::decodeBuffer(unsigned char buf[],
                      unsigned int  size)
{
    unsigned int pos = 0;
    unsigned int length = 0;

    if (!validateHeader(&buf[pos], size, length))
    {
        printf("%s: Invalid Header\n", __FUNCTION__);
        return false;
    }

    pos += length;

    while (pos < size)
    {
        if (!validateBlock(&buf[pos], size - pos, length))
        {
            return false;
        }
        pos += length;
    }

    return true;
}

bool
H17Disk::dumpBuffer(unsigned char buf[],
                    unsigned int  size,
                    int           level)
{
    unsigned int pos = 0;
    unsigned int length = 0;

    if (!validateHeader(&buf[pos], size, length))
    {
        printf("%s: Invalid Header\n", __FUNCTION__);
        return false;
    }

    pos += length;

    while (pos < size)
    {
        if (!dumpBlock(&buf[pos], size - pos, length, level))
        {
            return false;
        }
        pos += length;
    }

    return true;
}


//! load buffer into a h17disk object
//!
//! @param  buf
//! @param  size
//!
//! @return success
//!
bool
H17Disk::loadBuffer(unsigned char buf[],
                    unsigned int  size)
{
    unsigned int pos = 0;
    unsigned int length = 0;
    sectorErrs_m = 0;

    if (!loadHeader(&buf[pos], size, length))
    {
        return false;
    }

    pos += length;

    while (pos < size)
    {
        if (!loadBlock(&buf[pos], size - pos, length))
        {
            return false;
        }
        pos += length;
    }

    /*
    if (sectorErrs_m == 0)
    {
        printf("No sector errors.\n");
    }
    else
    {
        printf("Sector Errors: %d\n", sectorErrs_m);
    }
    */
    return true;
}


//! validate header of a h17disk file.
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param(out) length  length of processed data
//!
//! @return    if validation was succesful
//!
bool
H17Disk::validateHeader(unsigned char buf[],
                        unsigned int  size,
                        unsigned int &length)
{
    if (size < 7)
    {
        printf("Validate Header size too small: %d\n", size);
        return false;
    }

    // validate magic number
    if ((buf[0] != 'H') || (buf[1] != '1') || (buf[2] != '7') || (buf[3] != 'D'))
    {
       printf("Invalid Tag\n");
       return false;
    }

    // display file version number
    printf("H17D - Version: %d.%d.%d\n",buf[4], buf[5], buf[6]);

    length = 7;

    return true;
}


//! load header of h17disk file
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param(out) length  length of processed data
//!
//! @return   if validation of header was succesful
//!
bool
H17Disk::loadHeader(unsigned char buf[],
                    unsigned int  size,
                    unsigned int &length)
{
    if (size < 8)
    {
        printf("Validate Header size too small: %d\n", size);
        return false;
    }
    if ((buf[0] != 'H') || (buf[1] != '1') || (buf[2] != '7') || (buf[3] != 'D'))
    {
        printf("Invalid Tag\n");
        return false;
    }

    //printf("H17D - Version: %d.%d.%d\n",buf[4], buf[5], buf[6]);
    printf("H17D V%d.%d.%d\n",buf[4], buf[5], buf[6]);

    versionMajor_m = buf[4];
    versionMinor_m = buf[5];
    versionPoint_m = buf[6];


    bool validVersion = false;

    if (versionMajor_m == 1)
    {
        validVersion = true;
        length = 7;
    }

    if ((versionMajor_m == '2') && (buf[7] == 0xff)) {
        validVersion = true;
        length = 8;
    }

    return validVersion;
}

//! validate block
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param(out) length  length of processed data
//!
//! @return   if validation was succesful
//!
bool
H17Disk::validateBlock(unsigned char buf[],
                       unsigned int  size,
                       unsigned int &length)
{
    if (size < 6)
    {
        printf("Validate Block size too small: %d\n", size);
        return false;
    }
    // printf("Block ID: 0x%02x", buf[0]);
    // printf("   Flags: %s", (buf[1] & 0x80) ? "Mandatory": "Not Mandatory");
    unsigned int blockSize = ((unsigned int) buf[2] << 24) |
                             ((unsigned int) buf[3] << 16) |
                             ((unsigned int) buf[4] <<  8) |
                             ((unsigned int) buf[5]      );
    // printf("   Block Size: %u\n", blockSize);

    // make sure there is enough remaining data
    length = 6 + blockSize;
    if (size < length)
    {
        printf("Short file, block length: %d, but remaing file bytes:%d\n", length, size);
        return false;
    }

    // check block type
    switch (buf[0])
    {
        case DiskFormatBlock_c:
            validateDiskFormatBlock(&buf[6], blockSize);
            break;
        case FlagsBlock_c:
            validateFlagsBlock(&buf[6], blockSize);
            break;
        case LabelBlock_c:
            validateLabelBlock(&buf[6], blockSize);
            break;
        case CommentBlock_c:
            validateCommentBlock(&buf[6], blockSize);
            break;
        case DateBlock_c:
            validateDateBlock(&buf[6], blockSize);
            break;
        case ImagerBlock_c:
            validateImagerBlock(&buf[6], blockSize);
            break;
        case ProgramBlock_c:
            validateProgramBlock(&buf[6], blockSize);
            break;
        case DataBlock_c:
            validateDataBlock(&buf[6], blockSize);
            break;
        case RawDataBlock_c:
            validateRawDataBlock(&buf[6], blockSize);
            break;
        default:
            printf("Unknown Block Id: 0x%02x\n", buf[0]);
            //! \todo check to see if mandatory , and skip the data.
    }

    return true;
}

bool
H17Disk::dumpBlock(unsigned char buf[],
                   unsigned int  size,
                   unsigned int &length,
                   int           level)
{


    if (size < 6)
    {
        printf("Validate Block size too small: %d\n", size);
        return false;
    }
    // printf("Block ID: 0x%02x", buf[0]);
    // printf("   Flags: %s", (buf[1] & 0x80) ? "Mandatory": "Not Mandatory");
    unsigned int blockSize = ((unsigned int) buf[2] << 24) |
                             ((unsigned int) buf[3] << 16) |
                             ((unsigned int) buf[4] <<  8) |
                             ((unsigned int) buf[5]      );
    // printf("   Block Size: %u\n", blockSize);

    // make sure there is enough remaining data
    length = 6 + blockSize;
    if (size < length)
    {
        printf("Short file, block length: %d, but remaing file bytes:%d\n", length, size);
        return false;
    }

    // check block type
    switch (buf[0])
    {
        case DiskFormatBlock_c:
            validateDiskFormatBlock(&buf[6], blockSize);
            break;
        case FlagsBlock_c:
            validateFlagsBlock(&buf[6], blockSize);
            break;
        case LabelBlock_c:
            validateLabelBlock(&buf[6], blockSize);
            break;
        case CommentBlock_c:
            validateCommentBlock(&buf[6], blockSize);
            break;
        case DateBlock_c:
            validateDateBlock(&buf[6], blockSize);
            break;
        case ImagerBlock_c:
            validateImagerBlock(&buf[6], blockSize);
            break;
        case ProgramBlock_c:
            validateProgramBlock(&buf[6], blockSize);
            break;
        case DataBlock_c:
            validateDataBlock(&buf[6], blockSize);
            break;
        case RawDataBlock_c:
            validateRawDataBlock(&buf[6], blockSize);
            break;
        default:
            printf("Unknown Block Id: 0x%02x\n", buf[0]);
            //! \todo check to see if mandatory , and skip the data.
    }

    return true;

}

//! load block
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param(out) length  length of processed data
//!
//! @return    if validation was succesful
//!
bool
H17Disk::loadBlock(unsigned char buf[],
                   unsigned int  size,
                   unsigned int &length)
{

    H17Block *block = H17Block::create(buf, size);

    if (block)
    {
        uint8_t blockId = block->getBlockId();

        if (blocks_m[blockId])
        {
            // a label block already specified.
            //
            printf("Duplicate block: %d\n", blockId);
        }
        else
        {
            blocks_m[blockId] = block;
            length = block->getBlockSize();
        }
    }
    else
    {
        printf("Invalid File format: unable to create block\n");
        return false;
    }

    return true;
}


//! validate DiskFormatBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return   if validation was succesful
//!
bool
H17Disk::validateDiskFormatBlock(unsigned char buf[],
                                 unsigned int  size)
{
    if (size > 2)
    {
        printf("DataFormat size too big: %d\n", size);
        return false;
    }
    // set to default
    uint8_t sides = 1;
    uint8_t tracks = 40;
    if (size > 0)
    {
        sides = buf[0];
    }
    if (size > 1)
    {
        tracks = buf[1];
    }
    if (summarize_m) {
        printf("DataFormat - S: %d T: %d\n", sides, tracks);
    }
    else
    {
        printf("DataFormat:\n");
        printf("  Sides:  %d\n", buf[0]);
        printf("  Tracks: %d\n", buf[1]);
    }
    return true;
}


//! validate LabelBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return   if validation was succesful
//!
bool
H17Disk::validateLabelBlock(unsigned char buf[],
                            unsigned int  size)
{
    printf("Label:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        if (buf[i])
        {
            putchar(buf[i]);
        }
        else
        {
            printf("\n");
        }
    }
    printf("-------------------------------------------------------------------------\n");

    return true;
}


//! validate CommentBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return   if validate was successful
//!
bool
H17Disk::validateCommentBlock(unsigned char buf[],
                              unsigned int  size)
{
    if (summarize_m)
    {
        return true;
    }

    printf("Comment:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        if (buf[i])
        {
            putchar(buf[i]);
        }
        else
        {
            printf("\n");
        }
    }
    printf("-------------------------------------------------------------------------\n");

    return true;
}


//! validate DateBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return  if validate was successful
//!
bool
H17Disk::validateDateBlock(unsigned char buf[],
                           unsigned int  size)
{
    if (summarize_m)
    {
        return true;
    }

    printf("Date:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        if (buf[i])
        {
            putchar(buf[i]);
        }
        else
        {
            printf("\n");
        }
    }
    printf("-------------------------------------------------------------------------\n");

    return true;
}


//! validate ImagerBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return  if validation was successful
bool
H17Disk::validateImagerBlock(unsigned char buf[],
                             unsigned int  size)
{
    if (summarize_m)
    {
        return true;
    }
    printf("Imager:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        if (buf[i])
        {
            putchar(buf[i]);
        }
        else
        {
            printf("\n");
        }

    }
    printf("-------------------------------------------------------------------------\n");

    return true;
}


//! validate ProgramBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return   if validation was successful
//!
bool
H17Disk::validateProgramBlock(unsigned char buf[],
                              unsigned int  size)
{
    if (summarize_m)
    {
        return true;
    }
    printf("Program:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        if (buf[i])
        {
            putchar(buf[i]);
        }
        else
        {
            printf("\n");
        }
    }
    printf("-------------------------------------------------------------------------\n");

    return true;
}


//! validate FlagsBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return   if validation was successful
//!
bool
H17Disk::validateFlagsBlock(unsigned char buf[],
                            unsigned int size)
{

    printf("Flags Block:\n");
    if (size > 0)
    {
        printf("   R/O Flag:      %d\n", buf[0]);
    }
    if (size > 1)
    {
        printf("   Distribution:  %d\n", buf[1]);
    }
    if (size > 2)
    {
        printf("   Track Data:    %d\n", buf[2]);
    }
    for (unsigned int i = 3; i < size; i++)
    {
        printf("Flag %d - %s - %d\n", i, (buf[i] & 0x80) ? "Mandatory" : "Not Mandatory",
               buf[i] & 0x7f);
    }

    return true;
}


//! validate DataBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return  if validation was successful
//!
bool
H17Disk::validateDataBlock(unsigned char buf[],
                           unsigned int  size)
{
    printf("Data Block:\n");
    unsigned int pos = 0;
    unsigned int length;

    unsigned char tracks = 0;

    while ( pos < size )
    {
        if (buf[pos++] == TrackDataId)
        {
            validateTrackBlock(&buf[pos], size - pos, length);
            tracks++;
        }
        else
        {
            printf("Unknown subblock in Data Block: %d\n", buf[pos-1]);
            return false;
        }
        pos += length;
    }
    printf("Total tracks in DataBlock: %d\n", tracks);

    return true;
}


//! validate TrackBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param      length  bytes used in buffer for this block
//!
//! @return   if validation was successful
//!
bool
H17Disk::validateTrackBlock(unsigned char buf[],
                            unsigned int size,
                            unsigned int &length)
{
    printf("  Track Sub-Block:\n");
    if (size <  4)
    {
        printf("Insufficient space for track sub-block\n");
        return false;
    }
    printf("     Side:   %d\n", buf[0]);
    printf("     Track:  %d\n", buf[1]);
    curTrack_m = buf[1];
    unsigned int blockLength = (buf[2] << 8) | (buf[3]);
    unsigned int pos = 4;
    unsigned int len;
    printf("     Length: %d\n", blockLength);
    if (size < (blockLength + 4))
    {
        printf("Not enough space for track block: %d %d\n", size, blockLength+4);
        return false;
    }
    length = blockLength + 4;
    while ( pos < length )
    {
        if (buf[pos++] == SectorDataId)
        {
            validateSectorBlock(&buf[pos], size - pos, len);
        }
        else
        {
            printf("Unknown subblock in Data Block: %d\n", buf[pos-1]);
            return false;
        }
        pos += len;
    }

    return true;
}


//! validate SectorBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param      length  bytes used in buffer for this block
//!
//! @return   if validation was successful
//!
bool
H17Disk::validateSectorBlock(unsigned char buf[],
                             unsigned int size,
                             unsigned int &length)
{
    printf("    Sector Sub-Block:\n");
    if (size <  4)
    {
        printf("Insufficient space for sector sub-block\n");
        return false;
    }
    printf("        Sector:   %d\n", buf[0]);
    if (buf[1])
    {
        printf("        Error:    %d(on Track: %d) - %s\n", buf[1], curTrack_m,  sectorErrorStrings[buf[1]]);
    }
    else
    {
        printf("        Sector Valid\n");
    }

    unsigned int blockLength = (buf[2] << 8) | (buf[3]);
    printf("        Length: %d\n", blockLength);

    if (buf[1] != 0)
    {
        sectorErrs_m++;
    }

    printf("      Data:---------------------------------------------------------------\n");
    dumpDataBlock(buf, blockLength);
    unsigned int pos;

    printf("        ------------------------------------------------------------------\n");

    // dump actual sector data.
    pos = 4;
    int headerPos = -1;
    int dataPos = -1;

    // Find header block
    while ((pos < blockLength) && (buf[pos] != 0xfd))
    {
        pos++;
    }

    if (pos < blockLength)
    {
        headerPos = pos;
    }
    // skip header.
    pos += 5;

    // find Data block
    while ((pos < blockLength) && (buf[pos] != 0xfd))
    {
        pos++;
    }

    if (pos < blockLength)
    {
        dataPos = pos;
    }

    // if header found display it.
    if ((headerPos != -1) && (headerPos < ((int) blockLength - 5)))
    {
         dumpSectorHeader(&buf[headerPos]);
    }
    else
    {
        printf("  sector header missing - headerPos: %d\n", headerPos);
    }

    // if data found display it
    if ((dataPos != -1) && (dataPos < ((int) blockLength - 258)))
    {
        dumpSectorData(&buf[dataPos+1]);
    }
    else
    {
        printf("  sector data missing - dataPos: %d\n", dataPos);
    }

    length = blockLength + 4;

    return true;
}


//! dump SectorHeader
//!
//! @param      buf     data buffer
//!
void
H17Disk::dumpSectorHeader(unsigned char buf[])
{
    uint8_t calculatedChecksum = 0;

    printf("    Sector Header\n");
    printf("       Volume: %3d\n", buf[1]);
    calculatedChecksum = updateChecksum(calculatedChecksum, buf[1]);
    printf("       Track:   %2d\n", buf[2]);
    calculatedChecksum = updateChecksum(calculatedChecksum, buf[2]);
    printf("       Sector:   %d\n", buf[3]);
    calculatedChecksum = updateChecksum(calculatedChecksum, buf[3]);
    if (buf[4] == calculatedChecksum)
    {
        printf("       Chksum: 0x%02x\n", buf[4]);
    }
    else
    {
        printf("       Chksum: 0x%02x (expected 0x%02x)\n", calculatedChecksum, buf[4]);
    }
}


//! dump SectorData
//!
//! @param      buf     data buffer
//!
void
H17Disk::dumpSectorData(unsigned char buf[])
{
    printf("    Sector Data:\n");
    uint8_t printAble[16];
    uint8_t calculatedChecksum = 0;


    for (unsigned int i = 0; i < 256; i++)
    {
        calculatedChecksum = updateChecksum(calculatedChecksum, buf[i]);
        printAble[i % 16] = isprint(buf[i]) ? buf[i] : '.';
        if  ((i % 16) == 0)
        {
            printf("        %03d: ", i);
        }
        printf("%02x", buf[i]);

        if ((i % 16) == 7)
        {
            printf(" ");
        }
        if ((i % 16) == 15)
        {
            printf("        |");
            for(int i = 0; i < 8; i++)
            {
                printf("%c", printAble[i]);
            }
            printf("  ");
            for(int i = 8; i < 16; i++)
            {
                printf("%c", printAble[i]);
            }

            printf("|\n");
        }
    }

    if (buf[256] == calculatedChecksum)
    {
        printf("       Valid Data Chksum: 0x%02x\n", buf[256]);
    }
    else
    {
        printf("       Chksum: 0x%02x (expected 0x%02x)\n", calculatedChecksum, buf[256]);
    }
}


//! validate RawDataBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//!
//! @return  if load was successful
//!
bool
H17Disk::validateRawDataBlock(unsigned char buf[], unsigned int size)
{
    printf("Raw Data Block:\n");
    unsigned int pos = 0;
    unsigned int length;
    unsigned char tracks = 0;

    while ( pos < size )
    {
        if (buf[pos++] == RawTrackDataId)
        {
            validateRawTrackBlock(&buf[pos], size - pos, length);
            tracks++;
        }
        else
        {
            printf("Unknown subblock in Data Block: %d\n", buf[pos-1]);
            return false;
        }
        pos += length;
    }

    printf("Total tracks in Raw Data Block: %d\n", tracks);

    return true;
}


//! validate RawTrackBlock
//!
//! @param      buf     data buffer
//! @param      size    size of buffer
//! @param      length  length of buffer used
//!
//! @return  if load was successful
//!
bool
H17Disk::validateRawTrackBlock(unsigned char  buf[],
                               unsigned int   size,
                               unsigned int  &length)
{
    printf("  Raw Track Sub-Block:\n");
    if (size <  6)
    {
        printf("Insufficient space for raw track sub-block\n");
        return false;
    }
    unsigned int blockLength = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | buf[5];
    unsigned int pos = 6;
    unsigned int len;

    printf("    Side:   %d\n", buf[0]);
    printf("    Track:  %d\n", buf[1]);
    printf("    Length: %d\n", blockLength);
    length = blockLength + 6;

    while ( pos < length )
    {
        if (buf[pos++] == RawSectorDataId)
        {
            validateRawSectorBlock(&buf[pos], size - pos, len);
        }
        else
        {
            printf("Unknown subblock in Raw Track Block: %d\n", buf[pos-1]);
            return false;
        }
        pos += len;
    }

    return true;
}


//! print binary number
//!
//! @param     val     value to print in binary
//!
void
printBinary(unsigned char val)
{
    for (int i = 0; i < 8; i++)
    {
        printf("%d", (val & 0x80) >> 7);
        val <<= 1;
    }
    printf("#");
}


//! validate raw sector block
//!
//! @param buf
//! @param size
//! @param length
//!
//! @return success
//!
bool
H17Disk::validateRawSectorBlock(unsigned char  buf[],
                                unsigned int   size,
                                unsigned int  &length)
{
    printf("   Raw Sector Sub-Block:\n");
    if (size <  3)
    {
        printf("Insufficient space for sector sub-block\n");
        return false;
    }
    unsigned int blockLength = (buf[1] << 8) | (buf[2]);

    printf("     Sector:   %d\n", buf[0]);
    printf("     Length: %d\n", blockLength);

#if DUMP_DATA
    printf("   Data: ");
    for (unsigned int i = 3; i < blockLength + 3; i++)
    {
        printBinary(buf[i]);
    }
    printf("\n");
#else
    printf("   Raw Data: \n");
    dumpDataBlock(&buf[3], blockLength);
#endif

#if 1
    //unsigned char decode[length]; // should be length/2 but don't care about memory right now.
    //unsigned char *buff = new unsigned char[blockLength];
    unsigned char buff[blockLength];
    Decode::decodeFM(buff, &buf[3], blockLength >> 1);

    printf("  Data: \n");
    dumpDataBlock(buff, blockLength >> 1);

#if 1
    //unsigned char *finalBuff = new unsigned char[(length >> 1) + 5];
    unsigned char finalBuff[blockLength >> 1];

    processSector(buff, finalBuff, blockLength >> 1,  0, 0, 0);
    printf("  Aligned Data: \n");
    dumpDataBlock(finalBuff, blockLength >> 1);
    //delete[] finalBuff;
#endif
    //delete[] buff;
#endif
    length = blockLength + 3;

    return true;
}


//! write File Header
//!
//! @return success
//!
bool
H17Disk::writeHeader()
{
    unsigned char buf[7] = {
         'H',
         '1',
         '7',
         'D',
         versionMajor_m,
         versionMinor_m,
         versionPoint_m
    };

    if (!file_m.is_open())
    {
        return false;
    }

    file_m.write((const char*) buf, 7);

    return true;
}


//! set the number of sides
//!
//! @param sides
//!
//! @return success
//!
bool
H17Disk::setSides(unsigned char sides)
{
    sides_m = sides;

    return true;
}

//! set the number of tracks for the disk
//!
//! @param tracks
//!
//! @return success
//!
bool
H17Disk::setTracks(unsigned char tracks)
{
    tracks_m = tracks;

    return true;
}

//! write disk format block
//!
//! @return success
//!
bool
H17Disk::writeDiskFormatBlock()
{
    writeBlockHeader(DiskFormatBlock_c, 0x80, 2);
    unsigned char buf[2] = { sides_m, tracks_m };
    file_m.write((const char*) buf, 2);

    return true;
}

//! set write-protect for disk
//!
//! @param val
//!
//! @return success
//!
bool
H17Disk::setWPParameter(bool val)
{
    writeProtect_m = val;

    return true;
}

//! set distribution parameter
//!
//! @param val
//!
//! @return success
//!
bool
H17Disk::setDistributionParameter(unsigned char val)
{
    distribution_m = val;

    return true;
}

//! set track data parameter
//!
//! @param val
//!
//! @return success
//!
bool
H17Disk::setTrackDataParameter(unsigned char val)
{
    trackDataSource_m = val;

    return true;
}

//! write parameter block
//!
//! @return success
//!
bool
H17Disk::writeParameters()
{
    if (!file_m.is_open())
    {
        return false;
    }

    writeBlockHeader(FlagsBlock_c, 0x80, 3);


    unsigned char buf[3] = { writeProtect_m, distribution_m, trackDataSource_m };

    file_m.write((const char*) buf, 3);

    return true;
}


//! close file
//!
//! @return success
//!
bool
H17Disk::closeFile(void)
{
    if (!file_m.is_open())
    {
        return false;
    }

    file_m.close();

    return true;
}


//! Common code to write the block header
//!
//! @param blockId     Block ID
//! @param flag        Flag Byte
//! @param length      length of block
//!
//! @return  if successful
//!
bool
H17Disk::writeBlockHeader(uint8_t  blockId,
                          uint8_t  flag,
                          uint32_t length)
{
    unsigned char buf[6];

    buf[0] = blockId;
    buf[1] = flag;
    buf[2] = (length >> 24) & 0xff;
    buf[3] = (length >> 16) & 0xff;
    buf[4] = (length >>  8) & 0xff;
    buf[5] = length & 0xff;

    file_m.write((const char*) buf, 6);

    return true;
}


//! write label block
//!
//! @param buf
//! @param length
//!
//! @return success
//!
bool
H17Disk::writeLabel(unsigned char *buf,
                    uint32_t       length)
{
    writeBlockHeader(LabelBlock_c, 0x00, length);

    file_m.write((const char*) buf, length);

    return true;
}


//! write comment block
//!
//! @param buf
//! @param length
//!
//! @return success
//!
bool
H17Disk::writeComment(unsigned char *buf,
                      uint32_t       length)
{
    writeBlockHeader(CommentBlock_c, 0x00, length);

    file_m.write((const char*) buf, length);

    return true;
}


//! write date block
//!
//! @param buf
//! @param length
//!
//! @return success
//!
bool
H17Disk::writeDate(unsigned char *buf,
                   uint32_t       length)
{
    writeBlockHeader(DateBlock_c, 0x00, length);

    file_m.write((const char*) buf, length);

    return true;
}


//! write imager block
//!
//! @param buf
//! @param length
//!
//! @return success
//!
bool
H17Disk::writeImager(unsigned char *buf,
                     uint32_t       length)
{
    writeBlockHeader(ImagerBlock_c, 0x00, length);

    file_m.write((const char*) buf, length);

    return true;
}


//! write program
//!
//! @param buf
//! @param length
//!
//! @return success
//!
bool
H17Disk::writeProgram(unsigned char *buf,
                      uint32_t       length)
{
    writeBlockHeader(ProgramBlock_c, 0x00, length);

    file_m.write((const char*) buf, length);

    return true;
}


//! start data block
//!
//! @return success
//!
bool
H17Disk::startData()
{
    dataSize_m = 0;

    writeBlockHeader(DataBlock_c, 0x80, 0);
    dataBlockSizePos_m = file_m.tellp() - (streampos) 4;
    for (int i = 0; i < maxSectors_c; i++)
    {
        curTrackSectors_m[i] = nullptr;
    }

    return true;
}


//! start track for data.
//!
//! @param side
//! @param track
//!
//! @return success
//!
bool
H17Disk::startTrack(unsigned char side,
                    unsigned char track)
{
    //printf("%s: side: %d track: %d\n", __FUNCTION__, side, track);

    // validate sector data is clear
    for (int i = 0; i < maxSectors_c; i++)
    {
        if (curTrackSectors_m[i])
        {
            printf("Invalid data left on previous track: %d   sector: %d\n",
                   curTrack_m, i);
            curTrackSectors_m[i] = nullptr;
        }
    }

    // Handle the process block
    unsigned char buf[5] = { TrackDataId, side, track, 0, 0};

    file_m.write((const char*) buf, 5);
    trackSizePos_m = file_m.tellp() - (streampos) 2;
    curSide_m = side;
    curTrack_m = track;

    // Handle the raw blocks.
    //
    if (disableRaw_m)
    {
        return true;
    }

    // add a new 'rawTrack' to the disk
    rawTracks_m.push_back(new RawTrack(side, track));

    return true;
}


//! add a sector to the track
//!
//! @param sector
//! @param error - error code
//! @param buf - pointer to buffer
//! @param length - length of buffer
//!
//! @return success
//!
bool
H17Disk::addSector(unsigned char  sector,
                   unsigned char  error,
                   unsigned char *buf,
                   uint16_t       length)
{
    //printf("%s: Adding: %d - %d\n", __FUNCTION__, sector, length);
    if( curTrackSectors_m[sector])
    {
        printf("%s: Dup: %d\n", __FUNCTION__, sector);

        // unexpected data already for given sector
        return false;
    }

    curTrackSectors_m[sector] = new Sector(curSide_m, curTrack_m, sector, error, buf, length);

    return true;
}


//! end track and update size of track
//!
//! @return success
//!
bool
H17Disk::endTrack()
{
    //printf("%s: side: %d track: %d\n", __FUNCTION__, curSide_m, curTrack_m);
    // make sure all sectors are accounted for.
    for (int i = 0; i < maxSectors_c; i++)
    {
        if (!curTrackSectors_m[i])
        {
            // missing data for sector
            printf("Side: %d Track %d - Missing data for sector: %d\n", curSide_m, curTrack_m, i);
            // fill out an empty block.
            curTrackSectors_m[i] = new Sector(curSide_m,
                                              curTrack_m,
                                              i,
                                              1 /* todo - define Err_ReadError*/,
                                              nullptr,
                                              0);
        }
    }

    // write sectors in order
    for (int i = 0; i < maxSectors_c; i++)
    {
        curTrackSectors_m[i]->writeToFile(file_m);
        // free space
        delete curTrackSectors_m[i];
        curTrackSectors_m[i] = nullptr;
    }

    // update track size field in block.
    streampos curPos = file_m.tellp();
    uint16_t length = (uint16_t) (curPos - trackSizePos_m - 2);

    unsigned char buf[2] = { (unsigned char) ((length >> 8) & 0xff),
                             (unsigned char) (length & 0xff) };
    file_m.seekp(trackSizePos_m, ios::beg);
    file_m.write((const char*)buf, 2);
    file_m.seekp(curPos, ios::beg);

    return true;
}


//! end data block and update size
//!
//! @return success
//!
bool
H17Disk::endDataBlock()
{
    // go back and write the size of the data block to the header
    streampos curPos = file_m.tellp();
    uint32_t length = (uint32_t) (curPos - dataBlockSizePos_m - 4);

    unsigned char buf[4] = { (unsigned char) ((length >> 24) & 0xff),
                             (unsigned char) ((length >> 16) & 0xff),
                             (unsigned char) ((length >>  8) & 0xff),
                             (unsigned char)  (length        & 0xff) };

    file_m.seekp(dataBlockSizePos_m, ios::beg);
    file_m.write((const char*)buf, 4);

    // go to next byte position after data block.
    file_m.seekp(curPos, ios::beg);

    return true;
}


//! write raw data block
//!
//! @return success
//!
bool
H17Disk::writeRawDataBlock()
{
    if (disableRaw_m)
    {
        return true;
    }
    // header with a zero size
    unsigned char buf[6] = { RawDataBlock_c, 0x00, 0x00, 0x00, 0x00, 0x00 };

    file_m.write((const char *) buf, 6);

    // determine spot to write the size, once it's determined.
    streampos rawSizePos = file_m.tellp() - (streampos) 4;

    // write all tracks, they will recursively write the sectors.
    for (unsigned int i = 0 ; i < rawTracks_m.size(); i++)
    {
        rawTracks_m[i]->writeToFile(file_m);
    }

    // determine current position, to calculate size of block
    streampos curPos = file_m.tellp();
    uint32_t length = (uint32_t) (curPos - rawSizePos - 4);
    unsigned char buf2[4] = { (unsigned char) ((length >> 24) & 0xff),
                              (unsigned char) ((length >> 16) & 0xff),
                              (unsigned char) ((length >>  8) & 0xff),
                              (unsigned char)  (length        & 0xff) };

    // position to the size field
    file_m.seekp(rawSizePos, ios::beg);
    file_m.write((const char*) buf2, 4);

    // position to next byte after this block.
    file_m.seekp(curPos, ios::beg);

    return true;
}


//! add raw data sector
//!
//! @param sector
//! @param buf
//! @param length
//
//! @return success
//!
bool
H17Disk::addRawSector(uint8_t    sector,
                      uint8_t   *buf,
                      uint16_t   length)
{
    if (disableRaw_m)
    {
        return true;
    }

    // allocate a new raw sector, and store it to the current track.
    RawSector *tmp = new RawSector(curSide_m, curTrack_m, sector, buf, length);
    uint8_t trackPos = curTrack_m;

    if (sides_m == 2)
    {
        trackPos = trackPos << 1;
        trackPos += curSide_m;
    }
    rawTracks_m[trackPos]->addRawSector(tmp);

    return true;
}


//! add sector to data block
//!
//! @param side
//! @param track
//! @param sector
//! @param buf
//! @param length
//!
//! \todo implememnt
bool
H17Disk::addSectorToDataBlock(uint8_t   side,
                              uint8_t   track,
                              uint8_t   sector,
                              uint8_t  *buf,
                              uint16_t  length)
{
    if (!blocks_m[DataBlock_c])
    {
        return false;
    }

    //blocks_m[DataBlock_c]->addSector(side, track, sector, buf, length);

    return true;
}


//! add raw sector to data block
//!
//! @param side
//! @param track
//! @param sector
//! @param buf
//! @param length
//!
//! \todo implememnt
bool
H17Disk::addRawSectorToDataBlock(uint8_t   side,
                                 uint8_t   track,
                                 uint8_t   sector,
                                 uint8_t  *buf,
                                 uint16_t  length)
{
    if (!blocks_m[RawDataBlock_c])
    {
        return false;
    }

    //blocks_m[RawDataBlock_c]->addSector(side, track, sector, buf, length);

    return true;
}


//! get sector data
//!
//! @param side
//! @param track
//! @param sector
//!
//! @return pointer to buffer
//!
//! \todo needs to return both the pointer and length.
//! \todo implememnt
char *
H17Disk::getSectorData(unsigned char side,
                       unsigned char track,
                       unsigned char sector)
{

    return 0;
}


//! set default values for a h17disk file format
//!
//! @return success
//!
bool
H17Disk::setDefaults()
{
    bool retVal = true;

    if (!this->setDefaultDiskFormat())
    {
        retVal = false;
    }

    if (!this->setDefaultFlags())
    {
        retVal = false;
    }

    return retVal;
}


//! set default disk parameters
//!
//! @return success
//!
bool
H17Disk::setDefaultDiskFormat()
{
    sides_m = 1;
    tracks_m = 40;

    return true;
}


//! set default flags
//!
//! @return success
//!
bool
H17Disk::setDefaultFlags()
{
    distribution_m = DistUnknown;
    trackDataSource_m = TrackDataUnknown;
    writeProtect_m = false;

    return true;
}


H17Block *
H17Disk::getH17Block(uint8_t blockId)
{
    return blocks_m[blockId];
}
