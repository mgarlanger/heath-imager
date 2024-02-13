//! \file h17v2block.h
//!
//! Classes to handle the various blocks in the h17disk version 2 image file.
//!

#ifndef __H17V2BLOCK_H__
#define __H17V2BLOCK_H__

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>

class Track;
class Sector;

class H17V2Block
{

public:
    H17V2Block();
    H17V2Block(uint8_t buf[],
               uint32_t size);
    virtual ~H17V2Block();

    virtual uint32_t getDataSize();
    virtual uint8_t *getData();
    virtual uint32_t getBlockSize();
    virtual uint32_t getHeaderSize();

    virtual bool writeBlockHeader(std::ofstream &file);
    virtual bool writeToFile(std::ofstream &file);
    virtual bool dump(uint8_t level = 5) = 0;
    virtual bool analyze();

    virtual bool getMandatory() = 0;
    virtual uint8_t getBlockId() = 0;
    virtual void printBlockName() = 0;

    static H17V2Block *create(uint8_t buf[],
                              uint32_t size);

    static const uint8_t DiskFormatBlock_c = 0x00;
    static const uint8_t FlagsBlock_c = 0x01;
    static const uint8_t LabelBlock_c = 0x02;
    static const uint8_t CommentBlock_c = 0x03;
    static const uint8_t DateBlock_c = 0x04;
    static const uint8_t ImagerBlock_c = 0x05;
    static const uint8_t ProgramBlock_c = 0x06;

    static const uint8_t DataBlock_c = 0x10;
    static const uint8_t RawDataBlock_c = 0x30;

    static const uint8_t TrackDataId = 0x11;
    static const uint8_t SectorDataId = 0x12;

    static const uint8_t RawTrackDataId = 0x31;
    static const uint8_t RawSectorDataId = 0x32;

protected:
    virtual bool dumpText();

    static const uint32_t blockHeaderSize_c = 6;

    uint32_t size_m;
    uint8_t *buf_m;
};

class H17V2DiskormatBlock : public H17V2Block
{

public:
    H17V2DiskormatBlock(uint8_t  buf[],
                        uint32_t size);
    H17V2DiskormatBlock(uint8_t sides,
                        uint8_t tracks);
    virtual ~H17V2DiskormatBlock();

    virtual uint8_t getBlockId();
    virtual bool writeToFile(std::ofstream &file);
    virtual bool getMandatory();
    virtual uint32_t getDataSize();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();
    virtual void printBlockName();

    virtual uint8_t getSides();
    virtual uint8_t getTracks();

private:
    uint8_t sides_m;
    uint8_t tracks_m;
};

class H17V2FlagsBlock : public H17V2Block
{
public:
    H17V2FlagsBlock(uint8_t  buf[],
                    uint32_t size);
    H17V2FlagsBlock(bool    writeProtect,
                    uint8_t distribution,
                    uint8_t trackSource);
    virtual ~H17V2FlagsBlock();

    virtual uint8_t getBlockId();
    virtual bool writeToFile(std::ofstream &file);
    virtual bool getMandatory();
    virtual uint32_t getDataSize();
    virtual bool analyze();
    virtual bool dump(uint8_t level = 5);
    virtual void printBlockName();

private:
    uint8_t roFlag_m;
    uint8_t distribution_m;
    uint8_t trackData_m;
};

class H17V2LabelBlock : public H17V2Block
{
public:
    H17V2LabelBlock(uint8_t  buf[],
                    uint32_t size);
    virtual ~H17V2LabelBlock();

    virtual uint8_t getBlockId();
    virtual bool getMandatory();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();
    virtual void printBlockName();

private:
};

class H17V2CommentBlock : public H17V2Block
{
public:
    H17V2CommentBlock(uint8_t  buf[],
                      uint32_t size);
    virtual ~H17V2CommentBlock();

    virtual uint8_t getBlockId();
    virtual bool getMandatory();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();
    virtual void printBlockName();

private:
};

class H17V2DateBlock : public H17V2Block
{
public:
    H17V2DateBlock(uint8_t  buf[],
                   uint32_t size);
    virtual ~H17V2DateBlock();

    virtual uint8_t getBlockId();
    virtual bool getMandatory();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();
    virtual void printBlockName();

private:
};

class H17V2ImagerBlock : public H17V2Block
{
public:
    H17V2ImagerBlock(uint8_t  buf[],
                     uint32_t size);
    virtual ~H17V2ImagerBlock();

    virtual uint8_t getBlockId();
    virtual bool getMandatory();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();
    virtual void printBlockName();

private:
};

class H17V2ProgramBlock : public H17V2Block
{
public:
    H17V2ProgramBlock(uint8_t  buf[],
                      uint32_t size);
    virtual ~H17V2ProgramBlock();

    virtual uint8_t getBlockId();
    virtual bool getMandatory();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();
    virtual void printBlockName();

private:
};

class H17V2DataBlock : public H17V2Block
{
public:
    H17V2DataBlock(uint8_t  buf[],
                   uint32_t size);
    virtual ~H17V2DataBlock();

    virtual uint8_t getBlockId();
    virtual bool writeToFile(std::ofstream &file);
    virtual bool getMandatory();
    virtual bool dump(uint8_t level = 5);
    virtual bool analyze();

    virtual void printBlockName();

    virtual bool writeAsH8D(std::ofstream &file);
    virtual bool writeAsRaw(std::ofstream &file);

    virtual Track *getTrack(uint8_t side,
                            uint8_t track);
    virtual Sector *getSector(uint8_t side,
                              uint8_t track,
                              uint8_t sector);
    virtual Sector *getSector(uint16_t sector);
    virtual uint16_t getErrorCount();

private:
    std::vector<Track *> tracks_m;
};

#endif
