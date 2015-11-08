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
    virtual bool         dump(uint8_t level);
    virtual bool         analyze();

    virtual bool         getMandatory() = 0;
    virtual uint8_t      getBlockId() = 0;
    virtual void         printBlockName() = 0;

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
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();
 
private:
    
    uint8_t sides_m;
    uint8_t tracks_m; 
};


class H17DiskFlagsBlock: public H17Block
{
public:

    H17DiskFlagsBlock(uint8_t buf[], uint32_t size);
    H17DiskFlagsBlock(bool writeProtect, uint8_t distribution, uint8_t trackSource);
    virtual ~H17DiskFlagsBlock();

    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual uint32_t     getDataSize();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:
    
    uint8_t roFlag_m;
    uint8_t distribution_m;
    uint8_t trackData_m;
 
};


class H17DiskLabelBlock: public H17Block
{
public:

    H17DiskLabelBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskLabelBlock();

    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:

};


class H17DiskCommentBlock: public H17Block
{
public:

    H17DiskCommentBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskCommentBlock();

    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:

};


class H17DiskDateBlock: public H17Block
{
public:

    H17DiskDateBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskDateBlock();

    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:

};


class H17DiskImagerBlock: public H17Block
{
public:

    H17DiskImagerBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskImagerBlock();
   
    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:

};


class H17DiskProgramBlock: public H17Block
{
public:

    H17DiskProgramBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskProgramBlock();
   
    virtual uint8_t      getBlockId();
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:

};


class H17DiskDataBlock: public H17Block
{
public:

    H17DiskDataBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskDataBlock();

    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual bool         analyze();

    virtual void         printBlockName();

    virtual bool         writeAsH8D(std::ofstream &file);
    virtual bool         writeAsRaw(std::ofstream &file);


private:
    std::vector<Track *> tracks_m;

};


class H17DiskRawDataBlock: public H17Block
{
public:

    H17DiskRawDataBlock(uint8_t buf[], uint32_t size);
    virtual ~H17DiskRawDataBlock();

    virtual uint8_t      getBlockId();
    virtual bool         writeToFile(std::ofstream &file);
    virtual bool         getMandatory();
    virtual bool         dump(uint8_t level);
    virtual void         printBlockName();

private:
    std::vector<RawTrack *> rawTracks_m;

};

#endif

