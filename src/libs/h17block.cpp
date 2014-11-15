
#include "h17block.h"

#include <cstring>

H17Block::H17Block(unsigned char buf[], unsigned int size)
{
    buf_m = new unsigned char[size];
    size_m = size;

    memcpy(buf_m, buf, size_m);

}


H17Block::~H17Block()
{
    delete[] buf_m;
}

unsigned int 
H17Block::getBlockSize()
{
    return size_m + blockHeaderSize_c;
}

unsigned int
H17Block::getDataSize()
{
    return size_m;
}



H17DiskFormatBlock::H17DiskFormatBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskFormatBlock::~H17DiskFormatBlock()
{

}

unsigned char
H17DiskFormatBlock::getBlockId()
{
    return DiskFormatBlock_c;
}

H17DiskFlagsBlock::H17DiskFlagsBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskFlagsBlock::~H17DiskFlagsBlock()
{

}

unsigned char
H17DiskFlagsBlock::getBlockId()
{
    return FlagsBlock_c;
}

H17DiskLabelBlock::H17DiskLabelBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskLabelBlock::~H17DiskLabelBlock()
{

}


unsigned char
H17DiskLabelBlock::getBlockId()
{
    return LabelBlock_c;
}


H17DiskCommentBlock::H17DiskCommentBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskCommentBlock::~H17DiskCommentBlock()
{

}

unsigned char
H17DiskCommentBlock::getBlockId()
{
    return CommentBlock_c;
}


H17DiskDateBlock::H17DiskDateBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskDateBlock::~H17DiskDateBlock()
{

}


unsigned char
H17DiskDateBlock::getBlockId()
{
    return DateBlock_c;
}



H17DiskImagerBlock::H17DiskImagerBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskImagerBlock::~H17DiskImagerBlock()
{

}


unsigned char
H17DiskImagerBlock::getBlockId()
{
    return ImagerBlock_c;
}



H17DiskProgramBlock::H17DiskProgramBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskProgramBlock::~H17DiskProgramBlock()
{

}


unsigned char
H17DiskProgramBlock::getBlockId()
{
    return ProgramBlock_c;
}



H17DiskDataBlock::H17DiskDataBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskDataBlock::~H17DiskDataBlock()
{

}

unsigned char
H17DiskDataBlock::getBlockId()
{
    return DataBlock_c;
}


H17DiskRawDataBlock::H17DiskRawDataBlock(unsigned char buf[], unsigned int size): H17Block::H17Block( buf, size)
{

}

H17DiskRawDataBlock::~H17DiskRawDataBlock()
{

}

unsigned char
H17DiskRawDataBlock::getBlockId()
{
    return RawDataBlock_c;
}


