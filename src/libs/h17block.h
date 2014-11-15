#ifndef __H17BLOCK_H__
#define __H17BLOCK_H__

#include <iostream>
#include <fstream>


class H17Block 
{
public:
    H17Block(unsigned char buf[], unsigned int size);
    virtual ~H17Block();

    virtual unsigned int getDataSize();
    virtual unsigned int getBlockSize();


//    virtual bool writeToFile(std::ofstream file) = 0;

    static const unsigned int blockHeaderSize_c = 6;

    virtual unsigned char getBlockId() = 0;


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

private:

    unsigned int    size_m;
    unsigned char  *buf_m;
    
};


class H17DiskFormatBlock: public H17Block
{
public:

   H17DiskFormatBlock(unsigned char buf[], unsigned int size);
   ~H17DiskFormatBlock();
   
   virtual unsigned char getBlockId();
private:

   
};

class H17DiskFlagsBlock: public H17Block
{
public:

   H17DiskFlagsBlock(unsigned char buf[], unsigned int size);
   ~H17DiskFlagsBlock();

   virtual unsigned char getBlockId();
private:


};

class H17DiskLabelBlock: public H17Block
{
public:

   H17DiskLabelBlock(unsigned char buf[], unsigned int size);
   ~H17DiskLabelBlock();

   virtual unsigned char getBlockId();
private:


};


class H17DiskCommentBlock: public H17Block
{
public:

   H17DiskCommentBlock(unsigned char buf[], unsigned int size);
   ~H17DiskCommentBlock();

   virtual unsigned char getBlockId();
private:


};

class H17DiskDateBlock: public H17Block
{
public:

   H17DiskDateBlock(unsigned char buf[], unsigned int size);
   ~H17DiskDateBlock();

   virtual unsigned char getBlockId();
private:


};

class H17DiskImagerBlock: public H17Block
{
public:

   H17DiskImagerBlock(unsigned char buf[], unsigned int size);
   ~H17DiskImagerBlock();
   
   virtual unsigned char getBlockId();
private:


};


class H17DiskProgramBlock: public H17Block
{
public:

   H17DiskProgramBlock(unsigned char buf[], unsigned int size);
   ~H17DiskProgramBlock();
   
   virtual unsigned char getBlockId();
private:


};


class H17DiskDataBlock: public H17Block
{
public:

   H17DiskDataBlock(unsigned char buf[], unsigned int size);
   ~H17DiskDataBlock();

   virtual unsigned char getBlockId();
private:


};



class H17DiskRawDataBlock: public H17Block
{
public:

   H17DiskRawDataBlock(unsigned char buf[], unsigned int size);
   ~H17DiskRawDataBlock();

   virtual unsigned char getBlockId();
private:


};



#endif

