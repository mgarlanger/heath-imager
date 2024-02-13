//! \file h17block.h
//!
//! Classes to handle the various blocks in the h17disk image file.
//!

#ifndef __H17BLOCK_H__
#define __H17BLOCK_H__

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>


class Track;
class Sector;
class RawTrack;


class H17Block 
{

public:
    H17Block();
    H17Block(uint8_t buf[], uint32_t size);
    virtual ~H17Block();

    virtual uint32_t     getDataSize();
    virtual uint8_t     *getData();
    virtual uint32_t     getBlockSize();
    virtual uint32_t     getHeaderSize();
    
    virtual bool         writeBlockHeader(std::ofstream &file);
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         dump(uint8_t level = 5) = 0;
    virtual bool         analyze();

    virtual bool         getMandatory() = 0;
    virtual uint8_t      getBlockId() = 0;
    virtual void         printBlockName() = 0;

    static H17Block *create(uint8_t buf[], uint32_t size);

    static const uint8_t DiskFormatBlock_c = 0x00;
    static const uint8_t FlagsBlock_c      = 0x01;
    static const uint8_t LabelBlock_c      = 0x02;
    static const uint8_t CommentBlock_c    = 0x03;
    static const uint8_t DateBlock_c       = 0x04;
    static const uint8_t ImagerBlock_c     = 0x05;
    static const uint8_t ProgramBlock_c    = 0x06;

    static const uint8_t DataBlock_c       = 0x10;
    static const uint8_t RawDataBlock_c    = 0x30;

    static const uint8_t TrackDataId       = 0x11;
    static const uint8_t SectorDataId      = 0x12;

    static const uint8_t RawTrackDataId    = 0x31;
    static const uint8_t RawSectorDataId   = 0x32;

protected:
    virtual bool         dumpText();

    static const uint32_t  blockHeaderSize_c = 6;

    uint32_t               size_m;
    uint8_t               *buf_m;
};


class H17DiskFormatBlock: public H17Block
{

public:

    H17DiskFormatBlock(uint8_t buf[], uint32_t size);
    H17DiskFormatBlock(uint8_t sides, uint8_t tracks);
    virtual ~H17DiskFormatBlock();
   
    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual uint32_t     getDataSize();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

    virtual uint8_t      getSides();
    virtual uint8_t      getTracks();
 
private:
 
    uint8_t sides_m;
    uint8_t tracks_m; 
};


class H17FlagsBlock: public H17Block
{
public:

    H17FlagsBlock(uint8_t buf[], uint32_t size);
    H17FlagsBlock(bool writeProtect, uint8_t distribution, uint8_t trackSource);
    virtual ~H17FlagsBlock();

    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual uint32_t     getDataSize();
    virtual bool         analyze();
    virtual bool         dump(uint8_t level = 5);
    virtual void         printBlockName();

private:
    
    uint8_t roFlag_m;
    uint8_t distribution_m;
    uint8_t trackData_m;
 
};


class H17LabelBlock: public H17Block
{
public:

    H17LabelBlock(uint8_t buf[], uint32_t size);
    virtual ~H17LabelBlock();

    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

private:

};


class H17CommentBlock: public H17Block
{
public:

    H17CommentBlock(uint8_t buf[], uint32_t size);
    virtual ~H17CommentBlock();

    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

private:

};


class H17DateBlock: public H17Block
{
public:

    H17DateBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DateBlock();

    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

private:

};


class H17ImagerBlock: public H17Block
{
public:

    H17ImagerBlock(uint8_t buf[], uint32_t size);
    virtual ~H17ImagerBlock();
   
    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

private:

};


class H17ProgramBlock: public H17Block
{
public:

    H17ProgramBlock(uint8_t buf[], uint32_t size);
    virtual ~H17ProgramBlock();
   
    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

private:

};


class H17DataBlock: public H17Block
{
public:

    H17DataBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DataBlock();

    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();

    virtual void         printBlockName();

    virtual bool         writeAsH8D(std::ofstream &file);
    virtual bool         writeAsRaw(std::ofstream &file);

    virtual Track *      getTrack(uint8_t side, uint8_t track);
    virtual Sector *     getSector(uint8_t side, uint8_t track, uint8_t sector);
    virtual Sector *     getSector(uint16_t sector);
    virtual uint16_t     getErrorCount();

private:
    std::vector<Track *> tracks_m;

};


class H17RawDataBlock: public H17Block
{
public:

    H17RawDataBlock(uint8_t buf[], uint32_t size);
    virtual ~H17RawDataBlock();

    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level = 5);
    virtual bool         analyze();
    virtual void         printBlockName();

private:
    std::vector<RawTrack *> rawTracks_m;

};

#endif
