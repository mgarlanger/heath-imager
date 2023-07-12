//! \file h17v2block.cpp
//!
//! Classes to handle the various blocks in the h17disk image file.
//!

#include "h17v2block.h"
#include "track.h"
#include "sector.h"
#include "raw_track.h"
#include "raw_sector.h"
#include "dump.h"

#include <cstring>

// H17V2Block

//! Default constructor
//!
H17V2Block::H17V2Block() : size_m(0),
                           buf_m(nullptr)
{
}

//! Constructor to initialize buffer and size
//!
//! @param buf - buffer
//! @param size - size of buffer
//!
H17V2Block::H17V2Block(uint8_t buf[], uint32_t size)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
  buf_m = new unsigned char[size];
  size_m = size;

  memcpy(buf_m, buf, size_m);
}

//! H17V2Block Destructor
//!
H17V2Block::~H17V2Block()
{
  // printf("%s\n", __PRETTY_FUNCTION__);
  if (buf_m)
  {
    delete[] buf_m;
  }
}

H17V2Block *
H17V2Block::create(uint8_t buf[], uint32_t size)
{
  H17V2Block *newBlock = nullptr;

  if (size < 6)
  {
    return newBlock;
  }

  unsigned int blockSize = ((unsigned int)buf[2] << 24) |
                           ((unsigned int)buf[3] << 16) |
                           ((unsigned int)buf[4] << 8) |
                           ((unsigned int)buf[5]);
  // printf("   Block Size: %u\n", blockSize);
  if (size < (6 + blockSize))
  {
    printf("Insufficient size: %d\n", size);
    return newBlock;
  }

  // check block type
  switch (buf[0])
  {
  case DiskFormatBlock_c:
    newBlock = new H17DiskFormatBlock(&buf[6], blockSize);
    break;
  case FlagsBlock_c:
    newBlock = new H17FlagsBlock(&buf[6], blockSize);
    break;
  case LabelBlock_c:
    newBlock = new H17LabelBlock(&buf[6], blockSize);
    break;
  case CommentBlock_c:
    newBlock = new H17CommentBlock(&buf[6], blockSize);
    break;
  case DateBlock_c:
    newBlock = new H17DateBlock(&buf[6], blockSize);
    break;
  case ImagerBlock_c:
    newBlock = new H17ImagerBlock(&buf[6], blockSize);
    break;
  case ProgramBlock_c:
    newBlock = new H17ProgramBlock(&buf[6], blockSize);
    break;
  case DataBlock_c:
    newBlock = new H17DataBlock(&buf[6], blockSize);
    break;
  default:
    printf("Unknown Block Id: 0x%02x\n", buf[0]);
    //! \todo check to see if mandatory , and skip the data.
  }

  return newBlock;
}

//! return header size in bytes
//!
//! @return header size in bytes
//!
uint32_t
H17V2Block::getHeaderSize()
{
  return blockHeaderSize_c;
}

//! return total block size in bytes
//!
//! @return total block size in bytes
//!
uint32_t
H17V2Block::getBlockSize()
{
  return getHeaderSize() + getDataSize();
}

//! return size of data portion of block in bytes
//!
//! @return size of data porition of block in bytes
//!
uint32_t
H17V2Block::getDataSize()
{
  return size_m;
}

//! get pointer to data buffer
//!
//! @return pointer
//!
uint8_t *
H17V2Block::getData()
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
bool H17V2Block::writeBlockHeader(std::ofstream &file)
{
  unsigned char buf[6];
  uint32_t size = getDataSize();

  buf[0] = getBlockId();
  buf[1] = getMandatory() ? 0x80 : 0x00;
  buf[2] = (size >> 24) & 0xff;
  buf[3] = (size >> 16) & 0xff;
  buf[4] = (size >> 8) & 0xff;
  buf[5] = size & 0xff;

  file.write((const char *)buf, 6);

  return true;
}

//! write block to file
//!
//! @param file - file to write to
//!
//! @return if successful
//!
bool H17V2Block::writeToFile(std::ofstream &file)
{
  writeBlockHeader(file);

  uint8_t *data = getData();
  uint32_t size = getDataSize();

  file.write((const char *)data, size);

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
/*bool
H17V2Block::dump(uint8_t level)
{
    printBlockName();
    if (buf_m)
    {
        dumpDataBlock(buf_m, size_m);
    }

    return true;
}*/

bool H17V2Block::analyze()
{
  printBlockName();
  printf("analyze not implemented\n");

  return true;
}

//! dump text
//!
//! @return true
//!
bool H17V2Block::dumpText()
{
  char lastCh;
  // printBlockName();
  // printf("-------------------------------------------------------------------------\n");
  for (unsigned int i = 0; i < size_m; i++)
  {
    lastCh = buf_m[i];
    putchar(lastCh);
  }
  if (lastCh != '\n')
  {
    putchar('\n');
  }
  // printf("\n-------------------------------------------------------------------------\n");

  return true;
}

// H17DiskFormatBlock

//! constructor
//!
//! @param buf
//! @param size
//!
H17DiskFormatBlock::H17DiskFormatBlock(uint8_t buf[],
                                       uint32_t size) : sides_m(1),
                                                        tracks_m(40)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
  switch (size)
  {
  default:
    // should be error, unexpected size
    // fallthrough
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
                                       uint8_t tracks) : sides_m(sides),
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
void H17DiskFormatBlock::printBlockName()
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
bool H17DiskFormatBlock::writeToFile(std::ofstream &file)
{
  writeBlockHeader(file);

  unsigned char buf[2] = {sides_m, tracks_m};

  file.write((const char *)buf, 2);

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
bool H17DiskFormatBlock::dump(uint8_t level)
{
  if (level == 0)
  {
    return true;
  }

  if (level < 3)
  {
    printf("Format: S:%d/T:%d\n", sides_m, tracks_m);
    return true;
  }
  printf(" Disk Format Block:\n");
  printf("=====================\n");
  printf("   Sides:  %d\n", sides_m);
  printf("   Tracks: %d\n", tracks_m);
  printf("=====================\n");

  return true;
}

bool H17DiskFormatBlock::analyze()
{
  // TODO validate expected sides 1 or 2 and tracks 40 or 80

  return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17DiskFormatBlock::getMandatory()
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

// H17FlagsBlock

/// \todo verify the default values

//! constructor
//!
//! @param roFlag
//! @param dist
//! @param trackSource
//!
H17FlagsBlock::H17FlagsBlock(bool roFlag,
                             uint8_t dist,
                             uint8_t trackSource) : roFlag_m(roFlag),
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
H17FlagsBlock::H17FlagsBlock(uint8_t buf[],
                             uint32_t size) : roFlag_m(0),
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
H17FlagsBlock::~H17FlagsBlock()
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

//! print block name
//!
void H17FlagsBlock::printBlockName()
{
  printf("  Flags\n");
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17FlagsBlock::getBlockId()
{
  return FlagsBlock_c;
}

//! get size of data portion of block
//!
//! @return size of block
//!
uint32_t
H17FlagsBlock::getDataSize()
{
  return 3;
}

//! write to file
//!
//! @param file
//!
//! @return success
//!
bool H17FlagsBlock::writeToFile(std::ofstream &file)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
  uint8_t buf[3] = {roFlag_m, distribution_m, trackData_m};

  writeBlockHeader(file);

  file.write((const char *)buf, 3);

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
bool H17FlagsBlock::dump(uint8_t level)
{
  if (level == 0)
  {
    return true;
  }
  if (level < 3)
  {
    printf("Flags: RO: %d Dist: %d Trk: %d\n", roFlag_m, distribution_m, trackData_m);
    return true;
  }
  printf(" Disk Flags Block:\n");
  printf("=====================\n");
  printf("   Read-Only:  %d\n", roFlag_m);
  printf("   Distribution: %d\n", distribution_m);
  printf("   Track Source: %d\n", trackData_m);
  printf("=====================\n");

  return true;
}

bool H17FlagsBlock::analyze()
{
  // TODO validate distribution enum and track enum

  return true;
}

//! get mandatory flag
//!
//! @return flag
//!
bool H17FlagsBlock::getMandatory()
{
  return true;
}

// H17LabelBlock

//! constructor
//!
//! @param buf
//! @param size
//!
H17LabelBlock::H17LabelBlock(uint8_t buf[], uint32_t size) : H17V2Block::H17V2Block(buf, size)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

//! destructor
//!
H17LabelBlock::~H17LabelBlock()
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

//! print block name
//!
void H17LabelBlock::printBlockName()
{
  printf("  Label\n");
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17LabelBlock::getBlockId()
{
  return LabelBlock_c;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17LabelBlock::getMandatory()
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
bool H17LabelBlock::dump(uint8_t level)
{
  if (level < 2)
  {
    return true;
  }
  if (level == 2)
  {
  }
  printf("Label:\n");
  // printf("=======================================\n");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  dumpText();
  // printf("=======================================\n");
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  return true;
}

bool H17LabelBlock::analyze()
{
  // TODO ? validate text

  return true;
}

// H17CommentBlock

H17CommentBlock::H17CommentBlock(uint8_t buf[], uint32_t size) : H17V2Block::H17V2Block(buf, size)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

H17CommentBlock::~H17CommentBlock()
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17CommentBlock::getBlockId()
{
  return CommentBlock_c;
}

void H17CommentBlock::printBlockName()
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
bool H17CommentBlock::dump(uint8_t level)
{
  if (level < 3)
  {
    return true;
  }
  printf(" Comment Block:\n");
  printf("=====================\n");
  dumpText();
  printf("=====================\n");

  return true;
}

bool H17CommentBlock::analyze()
{
  // TODO ? validate text

  return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17CommentBlock::getMandatory()
{
  return false;
}

// H17DateBlock

H17DateBlock::H17DateBlock(uint8_t buf[], uint32_t size) : H17V2Block::H17V2Block(buf, size)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

H17DateBlock::~H17DateBlock()
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
bool H17DateBlock::dump(uint8_t level)
{
  if (level < 3)
  {
    return true;
  }
  printf(" Date:\n");
  printf("=====================\n");
  dumpText();
  printf("=====================\n");

  return true;
}

bool H17DateBlock::analyze()
{
  // TODO ? validate text is valid date

  return true;
}

void H17DateBlock::printBlockName()
{
  printf("  Date\n");
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17DateBlock::getBlockId()
{
  return DateBlock_c;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17DateBlock::getMandatory()
{
  return false;
}

// H17ImagerBlock

H17ImagerBlock::H17ImagerBlock(uint8_t buf[], uint32_t size) : H17V2Block::H17V2Block(buf, size)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

H17ImagerBlock::~H17ImagerBlock()
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

void H17ImagerBlock::printBlockName()
{
  printf("  Imager\n");
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17ImagerBlock::getBlockId()
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
bool H17ImagerBlock::dump(uint8_t level)
{
  if (level < 3)
  {
    return true;
  }
  printf(" Imager Block:\n");
  printf("=====================\n");
  dumpText();
  printf("=====================\n");

  return true;
}

bool H17ImagerBlock::analyze()
{
  // TODO ? validate text

  return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17ImagerBlock::getMandatory()
{
  return false;
}

// H17ProgramBlock

H17ProgramBlock::H17ProgramBlock(uint8_t buf[], uint32_t size) : H17V2Block::H17V2Block(buf, size)
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

H17ProgramBlock::~H17ProgramBlock()
{
  // printf("%s\n", __PRETTY_FUNCTION__);
}

void H17ProgramBlock::printBlockName()
{
  printf("  Program\n");
}

//! get block id
//!
//! @return block id
//!
uint8_t
H17ProgramBlock::getBlockId()
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
bool H17ProgramBlock::dump(uint8_t level)
{
  if (level < 3)
  {
    return true;
  }
  printf(" Program Block:\n");
  printf("=====================\n");
  dumpText();
  printf("=====================\n");

  return true;
}

bool H17ProgramBlock::analyze()
{
  // TODO ? validate text

  return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17ProgramBlock::getMandatory()
{
  return false;
}

// H17DataBlock

// H17DataBlock::H17DataBlock(uint8_t buf[], uint32_t size): H17V2Block::H17V2Block( buf, size)
H17DataBlock::H17DataBlock(uint8_t buf[], uint32_t size)
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

H17DataBlock::~H17DataBlock()
{
  // printf("%s\n", __PRETTY_FUNCTION__);

  for (unsigned int i = 0; i < tracks_m.size(); i++)
  {
    if (tracks_m[i])
    {
      delete tracks_m[i];
      tracks_m[i] = 0;
    }
  }
}

void H17DataBlock::printBlockName()
{
  printf("  Data\n");
}

uint16_t
H17DataBlock::getErrorCount()
{
  uint16_t count = 0;

  for (unsigned int i = 0; i < tracks_m.size(); i++)
  {
    count += tracks_m[i]->getErrorCount();
  }

  return count;
}
Track *
H17DataBlock::getTrack(uint8_t side, uint8_t track)
{
  for (unsigned int i = 0; i < tracks_m.size(); i++)
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
H17DataBlock::getSector(uint8_t side, uint8_t track, uint8_t sector)
{
  Track *trk = getTrack(side, track);

  if (trk)
  {
    return trk->getSector(sector);
  }

  return nullptr;
}

Sector *
H17DataBlock::getSector(uint16_t sectorNum)
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
H17DataBlock::getBlockId()
{
  return DataBlock_c;
}

//! write data to a file
//!
//! @param file
//!
//! @return success
//!
bool H17DataBlock::writeToFile(std::ofstream &file)
{
  // printf("%s\n", __PRETTY_FUNCTION__);

  writeBlockHeader(file);

  for (unsigned int i = 0; i < tracks_m.size(); i++)
  {
    tracks_m[i]->writeToFile(file);
  }

  return true;
}

//! write data to a file in h8d format
//!
//! @param file
//!
//! @return success
//!
bool H17DataBlock::writeAsH8D(std::ofstream &file)
{
  // printf("%s\n", __PRETTY_FUNCTION__);

  for (unsigned int i = 0; i < tracks_m.size(); i++)
  {
    tracks_m[i]->writeH8D(file);
  }

  return true;
}

//! write data to a file as h17raw
//!
//! @param file
//!
//! @return success
//!
bool H17DataBlock::writeAsRaw(std::ofstream &file)
{
  // printf("%s\n", __PRETTY_FUNCTION__);

  for (unsigned int i = 0; i < tracks_m.size(); i++)
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
bool H17DataBlock::dump(uint8_t level)
{
  if (level == 0)
  {
    return true;
  }

  uint16_t ec = getErrorCount();

  if (ec)
  {
    printf("Data Block: Error Count: %d\n", ec);
  }
  else
  {
    printf("Data Block: No errors\n");
  }

  if (level < 3)
  {
    return true;
  }

  for (unsigned int i = 0; i < tracks_m.size(); ++i)
  {
    tracks_m[i]->dump(level);
  }

  return true;
}

//! analyze the tracks
//!
//! @return success
//!
bool H17DataBlock::analyze()
{
  // printBlockName();

  // int expectedTracks = 40;
  // int expectedSides = 1;

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

  /*for (int side = 0; side < expectedSides; ++side)
  {
      for(int track = 0; track < expectedTracks; ++track)
      {
          if (!trackValid[side][track])
          {
              printf("Invalid track - side: %d, track: %d\n", side, track);
          }
      }
  }*/
  return true;
}

//! get mandatory flag
//!
//! @return true
//!
bool H17DataBlock::getMandatory()
{
  return true;
}
