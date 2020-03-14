//! \file h17block.cpp
//!
//! Classes to handle the various blocks in the h17disk image file.
//!

#include "h17block.h"
#include "track.h"
#include "sector.h"
#include "raw_track.h"
#include "raw_sector.h"
#include "dump.h"

#include <cstring>

// H17Block

//! Default constructor
//!
H17Block::H17Block(): size_m(0),
                      buf_m(nullptr)
{

}

//! Constructor to initialize buffer and size
//!
//! @param buf - buffer
//! @param size - size of buffer
//!
H17Block::H17Block(uint8_t buf[], uint32_t size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    buf_m = new unsigned char[size];
    size_m = size;

    memcpy(buf_m, buf, size_m);
}

//! H17Block Destructor
//!
H17Block::~H17Block()
{
    //printf("%s\n", __PRETTY_FUNCTION__);
    if (buf_m)
    {
        delete[] buf_m;
    }
}

//! return header size in bytes
//!
//! @return header size in bytes
//!
uint32_t
H17Block::getHeaderSize()
{
    return blockHeaderSize_c;
}

//! return total block size in bytes
//!
//! @return total block size in bytes
//!
uint32_t
H17Block::getBlockSize()
{
    return getHeaderSize() + getDataSize();
}

//! return size of data portion of block in bytes
//!
//! @return size of data porition of block in bytes
//!
uint32_t
H17Block::getDataSize()
{
    return size_m;
}


//! get pointer to data buffer
//!
//! @return pointer
//!
uint8_t *
H17Block::getData()
{
    return buf_m;
}

//! Common code to write the block header
//!
//! @param blockId     Block ID
//! @param flag        Flag Byte
//! @param length      length of block
//!
//! @returns if successful
//!
bool
H17Block::writeBlockHeader(std::ofstream &file)
{
    unsigned char buf[6];
    uint32_t size = getDataSize();

    buf[0] = getBlockId();
    buf[1] = getMandatory() ? 0x80 : 0x00;
    buf[2] = (size >> 24) & 0xff;
    buf[3] = (size >> 16) & 0xff;
    buf[4] = (size >>  8) & 0xff;
    buf[5] =  size        & 0xff;

    file.write((const char*) buf, 6);

    return true;
}

//! write block to file
//!
//! @param file - file to write to
//!
//! @return if successful
//!
bool 
H17Block::writeToFile(std::ofstream &file)
{
    //printf("%s\n", __PRETTY_FUNCTION__);

    writeBlockHeader(file);

    uint8_t *data = getData();
    uint32_t size = getDataSize();

    file.write((const char *) data, size);

    return true;
}

//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17Block::dump(uint8_t level)
{
    printBlockName();
    if (buf_m) 
    {
        dumpDataBlock(buf_m, size_m);
    }

    return true;
}


bool
H17Block::analyze()
{
    printBlockName();
    printf("analyze not implemented\n");

 
    return true;
}

//! dump text 
//! 
//! @return true
//!
bool
H17Block::dumpText()
{
    printBlockName();
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size_m; i++)
    {
        putchar(buf_m[i]);
    }
    printf("\n-------------------------------------------------------------------------\n");

    return true;
}


// H17DiskFormatBlock


//! constructor
//!
//! @param buf
//! @param size
//!
H17DiskFormatBlock::H17DiskFormatBlock(uint8_t buf[],
                                       uint32_t size): sides_m(1),
                                                       tracks_m(40)
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    switch (size)
    {
        default:
             // should be error, unexpected size
        case 2:
             tracks_m = buf[1];
             // fallthrough
        case 1:
             sides_m = buf[0];
             break;
        case 0:
             break;
    }
}


//! constructor
//!
//! @param sides
//! @param tracks
//!
H17DiskFormatBlock::H17DiskFormatBlock(uint8_t sides,
                                       uint8_t tracks): sides_m(sides),
                                                        tracks_m(tracks)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}


//! destructor
//!
H17DiskFormatBlock::~H17DiskFormatBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}



//! print block name
//!
void
H17DiskFormatBlock::printBlockName()
{
    printf("  Format\n");
}


//! get block id
//!
//! @return id
//!
uint8_t
H17DiskFormatBlock::getBlockId()
{
    return DiskFormatBlock_c;
}

//! get sides
//!
//! @return sides
//!
uint8_t
H17DiskFormatBlock::getSides()
{
    return sides_m;
}

//! get block id
//!
//! @return id
//!
uint8_t
H17DiskFormatBlock::getTracks()
{
    return tracks_m;
}



//! write block to file
//!
//! @param file
//!
//! @return status
//!
bool
H17DiskFormatBlock::writeToFile(std::ofstream &file)
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    writeBlockHeader(file);

    unsigned char buf[2] = { sides_m, tracks_m };

    file.write((const char*) buf, 2);

    return true;
}


//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskFormatBlock::dump(uint8_t level)
{
   printf(" Disk Format Block:\n");
   printf("=====================\n");
   printf("   Sides:  %d\n", sides_m);
   printf("   Tracks: %d\n", tracks_m);
   printf("=====================\n");

   return true;
}


//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskFormatBlock::getMandatory() 
{
    return true;
}


//! get size of data portion of block
//!
//! @return size in bytes
//!
uint32_t
H17DiskFormatBlock::getDataSize()
{
    return 2;
}



// H17DiskFlagsBlock

/// \todo verify the default values

//! constructor
//!
//! @param roFlag
//! @param dist
//! @param trackSource
//!
H17DiskFlagsBlock::H17DiskFlagsBlock(bool roFlag,
                                     uint8_t dist,
                                     uint8_t trackSource): roFlag_m(roFlag),
                                                           distribution_m(dist),
                                                           trackData_m(trackSource)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

/// \todo verify the default values

//! constructor
//!
//! @param roFlag
//! @param dist
//! @param trackSource
//!
H17DiskFlagsBlock::H17DiskFlagsBlock(uint8_t buf[],
                                     uint32_t size): roFlag_m(0),
                                                     distribution_m(0),
                                                     trackData_m(0)
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    /// \todo handle flag block larger than 3, - check mandatory flag.
    switch (size)
    {
        default:
        case 3:
             trackData_m = buf[2];
             // fallthrough
        case 2:
             distribution_m = buf[1];
             // fallthrough
        case 1:
             roFlag_m = buf[0];
             break;
        case 0:
             break;
    }
}


//! destructor
//! 
H17DiskFlagsBlock::~H17DiskFlagsBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}


//! print block name
//!
void
H17DiskFlagsBlock::printBlockName()
{
    printf("  Flags\n");
}


//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskFlagsBlock::getBlockId()
{
    return FlagsBlock_c;
}


//! get size of data portion of block
//!
//! @return size of block
//!
uint32_t
H17DiskFlagsBlock::getDataSize()
{
    return 3;
}


//! write to file
//!
//! @param file
//!
//! @return success
//!
bool
H17DiskFlagsBlock::writeToFile(std::ofstream &file)
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    uint8_t buf[3] = { roFlag_m, distribution_m, trackData_m };

    writeBlockHeader(file);

    file.write((const char*) buf, 3);
    
    return true;
}


//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskFlagsBlock::dump(uint8_t level)
{
   printf(" Disk Flags Block:\n");
   printf("=====================\n");
   printf("   Read-Only:  %d\n", roFlag_m);
   printf("   Distribution: %d\n", distribution_m);
   printf("   Track Source: %d\n", trackData_m);
   printf("=====================\n");

   return true;
}


//! get mandatory flag
//!
//! @return flag
//!
bool
H17DiskFlagsBlock::getMandatory() 
{
    return true;
}

// H17DiskLabelBlock


//! constructor
//!
//! @param buf
//! @param size
//!
H17DiskLabelBlock::H17DiskLabelBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}


//! destructor
//! 
H17DiskLabelBlock::~H17DiskLabelBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}


//! print block name
//!
void
H17DiskLabelBlock::printBlockName()
{
    printf("  Label\n");
}


//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskLabelBlock::getBlockId()
{
    return LabelBlock_c;
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskLabelBlock::getMandatory() 
{
    return false;
}

//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskLabelBlock::dump(uint8_t level)
{
   printf(" Imager Block:\n");
   printf("=====================\n");
   dumpText();
   printf("=====================\n");

   return true;
}



// H17DiskCommentBlock

H17DiskCommentBlock::H17DiskCommentBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

H17DiskCommentBlock::~H17DiskCommentBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskCommentBlock::getBlockId()
{
    return CommentBlock_c;
}

void
H17DiskCommentBlock::printBlockName()
{
    printf("  Comment\n");
}


//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskCommentBlock::dump(uint8_t level)
{
   printf(" Comment Block:\n");
   printf("=====================\n");
   dumpText();
   printf("=====================\n");

   return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskCommentBlock::getMandatory() 
{
    return false;
}



// H17DiskDateBlock

H17DiskDateBlock::H17DiskDateBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

H17DiskDateBlock::~H17DiskDateBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskDateBlock::dump(uint8_t level)
{
   printf(" Date:\n");
   printf("=====================\n");
   dumpText();
   printf("=====================\n");

   return true;
}

void
H17DiskDateBlock::printBlockName()
{
    printf("  Date\n");
}


//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskDateBlock::getBlockId()
{
    return DateBlock_c;
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskDateBlock::getMandatory() 
{
    return false;
}



// H17DiskImagerBlock

H17DiskImagerBlock::H17DiskImagerBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

H17DiskImagerBlock::~H17DiskImagerBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

void
H17DiskImagerBlock::printBlockName()
{
    printf("  Imager\n");
}


//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskImagerBlock::getBlockId()
{
    return ImagerBlock_c;
}

//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskImagerBlock::dump(uint8_t level)
{
   printf(" Imager Block:\n");
   printf("=====================\n");
   dumpText();
   printf("=====================\n");

   return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskImagerBlock::getMandatory() 
{
    return false;
}



// H17DiskProgramBlock

H17DiskProgramBlock::H17DiskProgramBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

H17DiskProgramBlock::~H17DiskProgramBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

}

void
H17DiskProgramBlock::printBlockName()
{
    printf("  Program\n");
}


//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskProgramBlock::getBlockId()
{
    return ProgramBlock_c;
}

//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskProgramBlock::dump(uint8_t level)
{
   printf(" Program Block:\n");
   printf("=====================\n");
   dumpText();
   printf("=====================\n");

   return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskProgramBlock::getMandatory() 
{
    return false;
}



// H17DiskDataBlock

//H17DiskDataBlock::H17DiskDataBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
H17DiskDataBlock::H17DiskDataBlock(uint8_t buf[], uint32_t size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

    size_m = size;
  
    uint32_t length;
    uint32_t pos = 0;

    while (pos < size)
    {
        tracks_m.push_back(new Track(&buf[pos], size - pos, length));
        pos += length;
    }
     
}

H17DiskDataBlock::~H17DiskDataBlock()
{
    // printf("%s\n", __PRETTY_FUNCTION__);

    for (unsigned int i = 0 ; i < tracks_m.size(); i++)
    {
        if (tracks_m[i])
        {
            delete tracks_m[i];
            tracks_m[i] = 0;
        }
    }
    
}

void
H17DiskDataBlock::printBlockName()
{
    printf("  Data\n");
}


Track *
H17DiskDataBlock::getTrack(uint8_t side, uint8_t track)
{
    for (unsigned int i = 0 ; i < tracks_m.size(); i++)
    {
        if (tracks_m[i]->getTrackNumber() == track &&
            tracks_m[i]->getSideNumber() == side)
        {
            return tracks_m[i];
        }
    }

    return nullptr; 
}

Sector *
H17DiskDataBlock::getSector(uint8_t side, uint8_t track, uint8_t sector)
{
    Track *trk = getTrack(side, track);

    if (trk) {
       return trk->getSector(sector);
    }

    return nullptr; 
}

Sector *
H17DiskDataBlock::getSector(uint16_t sectorNum)
{
    // determine track/sector number based on sectorNum
    // TODO: how to map sector 0->1599 to side/track/sector

    uint8_t sectNum = sectorNum % 10;
    uint8_t trackNum = sectorNum / 10;

    return getSector(0, trackNum, sectNum);
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskDataBlock::getBlockId()
{
    return DataBlock_c;
}


//! write data to a file 
//!
//! @param file 
//!
//! @return success
//!
bool
H17DiskDataBlock::writeToFile(std::ofstream &file)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

    writeBlockHeader(file);

    for (unsigned int i = 0 ; i < tracks_m.size(); i++)
    {
        tracks_m[i]->writeToFile(file);
    }
    
    return true;
}


//! write data to a file 
//!
//! @param file 
//!
//! @return success
//!
bool
H17DiskDataBlock::writeAsH8D(std::ofstream &file)
{   
    // printf("%s\n", __PRETTY_FUNCTION__);
    
    for (unsigned int i = 0 ; i < tracks_m.size(); i++)
    {   
        tracks_m[i]->writeH8D(file);
    }
    
    return true;
}


//! write data to a file 
//!
//! @param file
//!
//! @return success
//!
bool
H17DiskDataBlock::writeAsRaw(std::ofstream &file)
{  
    // printf("%s\n", __PRETTY_FUNCTION__);
   
    for (unsigned int i = 0 ; i < tracks_m.size(); i++)
    {
        tracks_m[i]->writeRaw(file);
    }
   
    return true;
}


//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @return success
//!
bool
H17DiskDataBlock::dump(uint8_t level)
{

   return true;
}


//! analyze the tracks
//!
//! @return success
//!
bool
H17DiskDataBlock::analyze()
{
    printBlockName();
 
    int expectedTracks = 40;
    int expectedSides = 1;

    bool trackValid[2][80];
    for (unsigned int i = 0; i < 2; ++i) 
    {
        for (unsigned int j = 0; j < 80; ++j) 
        {
            trackValid[i][j] = false;
        }
    }
    
    for (unsigned int i = 0; i < tracks_m.size(); ++i)
    {
        tracks_m[i]->analyze(trackValid);        
    } 

    for (int side = 0; side < expectedSides; ++side)
    {
        for(int track = 0; track < expectedTracks; ++track)
        {
            if (!trackValid[side][track])
            {
                printf("Invalid track - side: %d, track: %d\n", side, track);
            }
        }
    }
    return true; 
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskDataBlock::getMandatory() 
{
    return true;
}



// H17DiskRawDataBlock

H17DiskRawDataBlock::H17DiskRawDataBlock(uint8_t buf[], uint32_t size): H17Block::H17Block( buf, size)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

    size_m = size;

    uint32_t length;
    uint32_t pos = 0;

    while (pos < size)
    {
        rawTracks_m.push_back(new RawTrack(&buf[pos], size - pos, length));
        pos += length;
    }
}

H17DiskRawDataBlock::~H17DiskRawDataBlock()
{
    // printf("%s: %lu\n", __PRETTY_FUNCTION__, rawTracks_m.size());

    for (unsigned int i = 0 ; i < rawTracks_m.size(); i++)
    {   
        if (rawTracks_m[i])
        {
            delete rawTracks_m[i];
            rawTracks_m[i] = 0;
        }
    }
}


void
H17DiskRawDataBlock::printBlockName()
{
    printf("  Raw Data\n");
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17DiskRawDataBlock::getBlockId()
{
    return RawDataBlock_c;
}

bool
H17DiskRawDataBlock::writeToFile(std::ofstream &file)
{
    // printf("%s\n", __PRETTY_FUNCTION__);

    writeBlockHeader(file);

    for (unsigned int i = 0 ; i < rawTracks_m.size(); i++)
    {   
        rawTracks_m[i]->writeToFile(file);
    }

    return true;

}


//! dump the block to stdout
//! 
//! @param level - detail level to display
//!            0 - none
//!            1 - minimal
//!            2 - some
//!            3 - lots
//!            4 - complete
//!
//! @returns success
//!
bool
H17DiskRawDataBlock::dump(uint8_t level)
{

   return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool
H17DiskRawDataBlock::getMandatory() 
{
    return false;
}

