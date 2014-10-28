
#include "h17disk.h"

#define H17DISK 1

using namespace std;

H17Disk::H17Disk(): sides_m(1),
                    tracks_m(40),
                    curSide_m(0),
                    curTrack_m(0),
                    distribution_m(0),
                    trackDataSource_m(2),
                    writeProtect_m(false),
                    disableRaw_m(false),
                    sectorErrs_m(0)
{

}

H17Disk::~H17Disk()
{
    int i;
    for( i = 0; i < 10; i++)
    {
        curTrackSectors_m[i] = nullptr;
    } 
    rawTracks_m.reserve(160);
}

void
H17Disk::disableRaw()
{
    disableRaw_m = true;
}

bool
H17Disk::openForWrite(char *name)
{
   
    // make sure file is not already open
    if (file_m.is_open())
    {
        file_m.close();
    }

    file_m.open(name, ios::out | ios::binary);

    return (file_m.is_open());
}

bool
H17Disk::openForRead(char *name)
{
    // make sure file is not already open
    if (inFile_m.is_open())
    {
        inFile_m.close();
    }

    inFile_m.open(name, ios::in | ios::binary);

    return (inFile_m.is_open());
}

bool
H17Disk::openForRecovery(char *name)
{
    bool           status = false;

    if (!openForRead(name))
    { 
        return status;
    }

 //   loadFile();
    return false;
}

#if 0
bool
H17Disk::loadFile()
{
    readHeader();
    while (
}
#endif

bool
H17Disk::decodeFile(char *name)
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

    if ((inputBuffer) && (inFile_m.read((char *) inputBuffer, size)))
    {
        status = decodeBuffer(inputBuffer, size);
    }
    delete[] inputBuffer;
    inFile_m.close();
    return status;
}

bool
H17Disk::decodeBuffer(unsigned char buf[], unsigned int size)
{
    unsigned int pos = 0;
    unsigned int length = 0;
    sectorErrs_m = 0;

    if (!validateHeader(&buf[pos], size, length))
    {
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

    if (sectorErrs_m == 0)
    {
        printf("No sector errors.\n");
    }
    else
    {
        printf("Sector Errors: %d\n", sectorErrs_m);
    }
    return true;
}

bool
H17Disk::validateHeader(unsigned char buf[], unsigned int size, unsigned int &length)
{
    if (size < 7)
    {
        printf("Validate Header size too small: %d\n", size);
        return false;
    }
    if ((buf[0] != 'H') || (buf[1] != '1') || (buf[2] != '7') || (buf[3] != 'D'))
    {
       printf("Invalid Tag\n");
       return false;
    }

    printf("H17D - Version: %d.%d.%d\n",buf[4], buf[5], buf[6]);

    length = 7;
    return true;
}

bool
H17Disk::validateBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{
    if (size < 6)
    {
        printf("Validate Block size too small: %d\n", size);
        return false;
    }
    printf("Block ID: 0x%02x", buf[0]);
    printf("   Flags: %s", (buf[1] & 0x80) ? "Mandatory": "Not Mandatory");
    unsigned int blockSize = ((unsigned int) buf[2] << 24) | ((unsigned int) buf[3] << 16) | 
                             ((unsigned int) buf[4] << 8) | ((unsigned int) buf[5]);
    printf("   Block Size: %u\n", blockSize);
    if (size < ( 6 + blockSize))
    {
        printf("Insufficient size: %d\n", size);
        return false;
    }
    length = 6 + blockSize;

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
    }
    return true;
}

bool
H17Disk::validateDiskFormatBlock(unsigned char buf[], unsigned int size)
{
    if (size > 2)
    {
        printf("DataFormat size too big: %d\n", size);
        return false;
    }
    printf("DataFormat:\n");
    if (size > 0)
    {
        printf("  Sides:  %d\n", buf[0]);
    }
    if (size > 1)
    {
        printf("  Tracks: %d\n", buf[1]);
    }
    return true;
}

bool
H17Disk::validateLabelBlock(unsigned char buf[], unsigned int size)
{
    printf("Label:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        putchar(buf[i]);
    }
    printf("-------------------------------------------------------------------------\n");
   return true;
}

bool
H17Disk::validateCommentBlock(unsigned char buf[], unsigned int size)
{
    printf("Comment:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        putchar(buf[i]);
    }
    printf("-------------------------------------------------------------------------\n");
   return true;
}

bool
H17Disk::validateDateBlock(unsigned char buf[], unsigned int size)
{
    printf("Date:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        putchar(buf[i]);
    }
    printf("-------------------------------------------------------------------------\n");
   return true;
}

bool
H17Disk::validateImagerBlock(unsigned char buf[], unsigned int size)
{
    printf("Imager:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        putchar(buf[i]);
    }
    printf("-------------------------------------------------------------------------\n");
   return true;
}

bool
H17Disk::validateProgramBlock(unsigned char buf[], unsigned int size)
{
    printf("Program:\n");
    printf("-------------------------------------------------------------------------\n");
    for (unsigned int i = 0; i < size; i++)
    {
        putchar(buf[i]);
    }
    printf("-------------------------------------------------------------------------\n");
   return true;
}

bool
H17Disk::validateFlagsBlock(unsigned char buf[], unsigned int size)
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

bool
H17Disk::validateDataBlock(unsigned char buf[], unsigned int size)
{
    printf("Data Block:\n");
    unsigned int pos = 0;
    unsigned int length; 

    while ( pos < size )
    {
        if (buf[pos++] == TrackDataId)
        {
            validateTrackBlock(&buf[pos], size - pos, length);
        }
        else
        {
            printf("Unknown subblock in Data Block: %d\n", buf[pos-1]);
            return false;
        }
        pos += length;
    }
    return true;
}

bool
H17Disk::validateTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{
    printf("  Track Sub-Block:\n");
    if (size <  4)
    {
        printf("Insufficient space for track sub-block\n");
        return false;
    }
    printf("     Side:   %d\n", buf[0]);
    printf("     Track:  %d\n", buf[1]);
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

bool
H17Disk::validateSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{
    printf("    Sector Sub-Block:\n");
    if (size <  4)
    {
        printf("Insufficient space for sector sub-block\n");
        return false;
    }
    printf("        Sector:   %d\n", buf[0]);
    printf("        Error:    %d\n", buf[1]);
    unsigned int blockLength = (buf[2] << 8) | (buf[3]);
    printf("        Length: %d\n", blockLength); 

    if (buf[1] != 0)
    {
        sectorErrs_m++;
    }

    printf("   Data:\n");

    //BYTE printAble[16];
    for (unsigned int i = 0; i < blockLength; i++)
    {
        if  ((i % 16) == 0)
        {
            printf("     %03d: ", i);
        }
        printf("%02x", buf[i]);

        if ((i % 16) == 7)
        {
            printf(" ");
        }
        if ((i % 16) == 15)
        {
            printf("\n");
        }
    }
    length = blockLength + 4; 
    return true;
}

bool
H17Disk::validateRawDataBlock(unsigned char buf[], unsigned int size)
{
    printf("Raw Data Block:\n");
    unsigned int pos = 0;
    unsigned int length;

    while ( pos < size )
    {
        if (buf[pos++] == RawTrackDataId)
        {
            validateRawTrackBlock(&buf[pos], size - pos, length);
        }
        else
        {
            printf("Unknown subblock in Data Block: %d\n", buf[pos-1]);
            return false;
        }
        pos += length;
    }
    return true;
}

bool
H17Disk::validateRawTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length)
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

void printBinary(unsigned char val)
{
    for (int i = 0; i < 8; i++)
    {
#if 0
        printf("%d", val & 0x01);
        val >>= 1;
#else
        printf("%d", (val & 0x80) >> 7);
        val <<= 1;
#endif

    } 
    printf("#");

}

bool
H17Disk::validateRawSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{
    printf("    Raw Sector Sub-Block:\n");
    if (size <  3)
    {
        printf("Insufficient space for sector sub-block\n");
        return false;
    }
    unsigned int blockLength = (buf[1] << 8) | (buf[2]);

    printf("      Sector:   %d\n", buf[0]);
    printf("      Length: %d\n", blockLength);

#if DUMP_DATA
    printf("   Data: ");
    for (unsigned int i = 3; i < blockLength + 3; i++)
    {
        printBinary(buf[i]);
    }
    printf("\n");
#endif

    length = blockLength + 3;
    
    return true;
}



// write Header
bool
H17Disk::writeHeader()
{
    unsigned char buf[7] = { 'H', '1', '7', 'D', 1, 0, 0 };

    if (!file_m.is_open())
    {
        return false;
    }
    
    file_m.write((const char*) buf, 7);

    return true;
}

bool
H17Disk::setSides(unsigned char sides)
{
    sides_m = sides;
    return true; 
}
bool
H17Disk::setTracks(unsigned char tracks)
{
    tracks_m = tracks;
    return true; 
}

bool
H17Disk::writeDiskFormatBlock()
{
    writeBlockHeader(0x00, 0x80, 2);
    unsigned char buf[2] = { sides_m, tracks_m };
    file_m.write((const char*) buf, 2);

    return true; 
}

bool
H17Disk::setWPParameter(bool val)
{
    writeProtect_m = val;
    return true; 
}

bool
H17Disk::setDistributionParameter(unsigned char val)
{
    distribution_m = val;
    return true; 
}

bool
H17Disk::setTrackDataParameter(unsigned char val)
{
    trackDataSource_m = val;
    return true; 
}

bool
H17Disk::writeParameters()
{
    if (!file_m.is_open())
    {
        return false;
    }

    writeBlockHeader(0x01, 0x80, 3);


    unsigned char buf[3] = { writeProtect_m, distribution_m, trackDataSource_m };

    file_m.write((const char*) buf, 3);

    return true; 
}

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

bool
H17Disk::writeBlockHeader(uint8_t blockId, uint8_t flag, uint32_t length)
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

bool
H17Disk::writeComment(unsigned char *buf, uint32_t length)
{
    writeBlockHeader(CommentBlock_c, 0x00, length);

    file_m.write((const char*) buf, length);
    return true;
}

bool
H17Disk::startData()
{
    dataSize_m = 0;

    writeBlockHeader(0x10, 0x80, 0);
    dataBlockSizePos_m = file_m.tellp() - (streampos) 4;
    return true;
}

bool
H17Disk::startTrack(unsigned char side,
                       unsigned char track)
{
    // validate sector data is clear
    for (int i = 0; i < maxSectors_c; i++)
    {
        if (curTrackSectors_m[i])
        {
            printf("Invalid data left on previous track: %d   sector: %d\n", 
                   curTrack_m, indexToSector(i));
            curTrackSectors_m[i] = nullptr;
        }
    }
    // Handle the process block
    unsigned char buf[5] = { 0x11, side, track, 0, 0};

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
    rawTracks_m.push_back(new RawTrack(side, track));    
    return true;
}

bool
H17Disk::addSector(unsigned char  sector,
                   unsigned char  error,
                   unsigned char *buf,
                   uint16_t       length)
{
    printf("%s: Adding: %d - %d\n", __FUNCTION__, sector, length);
    if( curTrackSectors_m[sectorToIndex(sector)])
    {
        printf("%s: Dup: %d\n", __FUNCTION__, sector);
        
        // unexpected data already for given sector
        return false;
    }

    curTrackSectors_m[sectorToIndex(sector)] = new Sector(curSide_m, curTrack_m, sector, error, buf, length);

    return true;
}


bool
H17Disk::endTrack()
{
    for (int i = 0; i < maxSectors_c; i++)
    {
        if (!curTrackSectors_m[i])
        {
            // missing data for sector
            printf("Missing data for sector: %d\n", indexToSector(i));
            curTrackSectors_m[i] = new Sector(curSide_m, curTrack_m, indexToSector(i), 
                                              1 /* TODO - define Err_ReadError*/, 
                                              nullptr, 0);
        }
    }

    for (int i = 0; i < maxSectors_c; i++)
    {
        curTrackSectors_m[i]->writeToFile(file_m);
        delete curTrackSectors_m[i];
        curTrackSectors_m[i] = nullptr;
    }
 
    streampos curPos = file_m.tellp();
    uint16_t length = (uint16_t) (curPos - trackSizePos_m - 2);

    unsigned char buf[2] = { (unsigned char) ((length >> 8) & 0xff), 
                             (unsigned char) (length & 0xff) };
    file_m.seekp(trackSizePos_m, ios::beg);
    file_m.write((const char*)buf, 2);
    file_m.seekp(curPos, ios::beg);
    return true;
}

bool
H17Disk::endDataBlock()
{
    streampos curPos = file_m.tellp();
    uint32_t length = (uint32_t) (curPos - dataBlockSizePos_m - 4);

    unsigned char buf[4] = { (unsigned char) ((length >> 24) & 0xff),
                             (unsigned char) ((length >> 16) & 0xff),
                             (unsigned char) ((length >>  8) & 0xff),
                             (unsigned char) (length         & 0xff) };
    file_m.seekp(dataBlockSizePos_m, ios::beg);
    file_m.write((const char*)buf, 4);
    file_m.seekp(curPos, ios::beg);
    return true;
}

bool
H17Disk::writeRawDataBlock()
{
    if (disableRaw_m)
    {
        return true;
    }
    unsigned char buf[6] = { 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 };
    file_m.write((const char *) buf, 6);
    streampos rawSizePos = file_m.tellp() - (streampos) 4;

    for (unsigned int i = 0 ; i < rawTracks_m.size(); i++)
    {
        rawTracks_m[i]->writeToFile(file_m);
    }

    streampos curPos = file_m.tellp();
    uint32_t length = (uint32_t) (curPos - rawSizePos - 4);
    unsigned char buf2[4] = { (unsigned char) ((length >> 24) & 0xff),
                              (unsigned char) ((length >> 16) & 0xff),
                              (unsigned char) ((length >>  8) & 0xff),
                              (unsigned char) (length         & 0xff) };
    file_m.seekp(rawSizePos, ios::beg);
    file_m.write((const char*)buf2, 4);
    file_m.seekp(curPos, ios::beg);
 
    return true;
}

bool
H17Disk::addRawSector(unsigned char  sector,
                           unsigned char *buf,
                           uint16_t       length)
{
    if (disableRaw_m)
    {
        return true;
    }
    RawSector *tmp = new RawSector(curSide_m, curTrack_m, sector, buf, length); 
    rawTracks_m[curTrack_m]->addRawSector(tmp);
    return true;
}

