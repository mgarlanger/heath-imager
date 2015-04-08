
#include "h17disk.h"
#include "disk_util.h"
#include "h17block.h"
#include "decode.h"

#define H17DISK 1

using namespace std;

// dumpDataBlock()
//
// @param title   Title to display
// @param buf     buffer data
// @param length  length of buffer
//
void dumpDataBlock(string title, unsigned char buf[], unsigned int length) {
    printf("   %s:\n", title.c_str());

    //BYTE printAble[16];
    for (unsigned int i = 0; i < length; i++)
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
    printf("\n");
}



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
    for( i = 0; i < 40; i++)
    {
        blocks_m[i] = nullptr;
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

bool
H17Disk::loadFile(char *name)
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
H17Disk::loadBuffer(unsigned char buf[], unsigned int size)
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


// validateHeader()
//
// @param      buf     data buffer
// @param      size    size of buffer
// @param(out) length  length of processed data
//
// @return   {bool} if validation was succesful
bool
H17Disk::validateHeader(unsigned char buf[], unsigned int size, unsigned int &length)
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

bool
H17Disk::loadHeader(unsigned char buf[], unsigned int size, unsigned int &length)
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

    // make sure there is enough remaining data
    if (size < ( 6 + blockSize))
    {
        printf("Insufficient size: %d\n", size);
        return false;
    }
    length = 6 + blockSize;

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
            /// \todo check to see if mandatory , and skip the data.
    }
    return true;
}

bool
H17Disk::loadBlock(unsigned char buf[], unsigned int size, unsigned int &length)
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
            loadDiskFormatBlock(&buf[6], blockSize);
            break;
        case FlagsBlock_c:
            loadFlagsBlock(&buf[6], blockSize);
            break;
        case LabelBlock_c:
            loadLabelBlock(&buf[6], blockSize);
            break;
        case CommentBlock_c:
            loadCommentBlock(&buf[6], blockSize);
            break;
        case DateBlock_c:
            loadDateBlock(&buf[6], blockSize);
            break;
        case ImagerBlock_c:
            loadImagerBlock(&buf[6], blockSize);
            break;
        case ProgramBlock_c:
            loadProgramBlock(&buf[6], blockSize);
            break;
        case DataBlock_c:
            loadDataBlock(&buf[6], blockSize);
            break;
        case RawDataBlock_c:
            loadRawDataBlock(&buf[6], blockSize);
            break;
        default:
            printf("Unknown Block Id: 0x%02x\n", buf[0]);
            /// \todo check to see if mandatory , and skip the data.
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
H17Disk::loadDiskFormatBlock(unsigned char buf[], unsigned int size)
{
    if (size > 2)
    {
        //printf("DataFormat size too big: %d\n", size);
        return false;
    }
    //printf("DataFormat:\n");
    if (size > 0)
    {
        sides_m = buf[0];
        if (sides_m > 2)
        {
            printf("Invalid Number of Sides:  %d\n", sides_m);
            return false;
        }
    }
    if (size > 1)
    {
        tracks_m = buf[1];
        /// \todo check track value
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
H17Disk::loadLabelBlock(unsigned char buf[], unsigned int size)
{
    if (blocks_m[LabelBlock_c])
    {
        // already a label block specified.
        //
        printf("Duplicate LabelBlock\n");
    }
    else
    {
        blocks_m[LabelBlock_c] = new H17DiskLabelBlock(buf, size);
        printf("New Label Block - Size: %d\n", size);
    }
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
H17Disk::loadCommentBlock(unsigned char buf[], unsigned int size)
{   
    if (blocks_m[CommentBlock_c])
    {   
        // already a comment block specified.
        //
        printf("Duplicate CommentBlock\n");
    }
    else
    {   
        blocks_m[CommentBlock_c] = new H17DiskCommentBlock(buf, size);
        printf("New Comment Block - Size: %d\n", size);
    }
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
H17Disk::loadDateBlock(unsigned char buf[], unsigned int size)
{   
    if (blocks_m[DateBlock_c])
    {   
        // already a date block specified.
        //
        printf("Duplicate DateBlock\n");
    }
    else
    {
        blocks_m[DateBlock_c] = new H17DiskDateBlock(buf, size);
        printf("New Date Block - Size: %d\n", size);
    }
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
H17Disk::loadImagerBlock(unsigned char buf[], unsigned int size)
{  
    if (blocks_m[ImagerBlock_c])
    {  
        // already a imager block specified.
        //
        printf("Duplicate ImagerBlock\n");
    }
    else
    {
        blocks_m[ImagerBlock_c] = new H17DiskImagerBlock(buf, size);
        printf("New Imager Block - Size: %d\n", size);
    }
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
H17Disk::loadProgramBlock(unsigned char buf[], unsigned int size)
{ 
    if (blocks_m[ProgramBlock_c])
    {
        // already a program block specified.
        //
        printf("Duplicate ProgramBlock\n");
    }
    else
    {
        blocks_m[ProgramBlock_c] = new H17DiskProgramBlock(buf, size);
        printf("New  ProgramBlock - Size: %d\n", size);
    }
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
H17Disk::loadFlagsBlock(unsigned char buf[], unsigned int size)
{
    if (blocks_m[FlagsBlock_c])
    {
        // already a flags block specified.
        //
        printf("Duplicate FlagsBlock\n");
    }
    else
    {
        blocks_m[FlagsBlock_c] = new H17DiskFlagsBlock(buf, size);
        printf("New  FlagsBlock - Size: %d\n", size);
    }
    return true;
}



bool
H17Disk::validateDataBlock(unsigned char buf[], unsigned int size)
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


bool
H17Disk::loadDataBlock(unsigned char buf[], unsigned int size)
{
    if (blocks_m[DataBlock_c])
    {
        // already a label block specified.
        //
        printf("Duplicate DataBlock\n");
    }
    else
    {
        blocks_m[DataBlock_c] = new H17DiskDataBlock(buf, size);
        printf("New  DataBlock - Size: %d\n", size);
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
H17Disk::loadTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{   
    if (blocks_m[TrackDataId])
    {   
        // already a label block specified.
        //
        printf("Duplicate TrackBlock\n");
    }
    else
    {   
//        blocks_m[TrackBlock_c] = new H17DiskTrackBlock(buf, size);
        printf("New  TrackBlock - Size: %d\n", size);
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

    printf("      Data:---------------------------------------------------------------\n");

    uint8_t printAble[16];
    unsigned int i = 0;
    unsigned int pos = 4;
    for (i = 0; i < blockLength - 4; i++)
    {
        printAble[i % 16] = isprint(buf[pos]) ? buf[pos] : '.';
        if  ((i % 16) == 0)
        {
            printf("        %03d: ", i);
        }
        printf("%02x", buf[pos]);

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
        pos++;
    } 
    if ((i % 16) != 0) 
    {
        unsigned int pos = i % 16;
        unsigned int numSpaces = (16 - pos ) * 2 + ((16 - pos) >> 3);
        while(numSpaces--) {
            printf(" ");
        }
        printf("        |");
        for(unsigned int i = 0; i < 8; i++)
        {
            if (i < pos)
            {
                printf("%c", printAble[i]);
            }
            else 
            {
                printf(" ");
            }
        }
        printf("  ");
        for(unsigned int i = 8; i < 16; i++)
        {
            if (i < pos)
            {
                printf("%c", printAble[i]);
            }
            else
            {
                printf(" ");
            }
        }

        printf("|\n");
    } 
    printf("\n");
    printf("        ------------------------------------------------------------------\n");
    
    // dump actual track data.
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


bool
H17Disk::loadSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{   
    if (blocks_m[SectorDataId])
    {
        // already a label block specified.
        //
        printf("Duplicate TrackBlock\n");
    }
    else
    {   
//        blocks_m[TrackBlock_c] = new H17DiskTrackBlock(buf, size);
        printf("New  TrackBlock - Size: %d\n", size);
    }
    return true;
}


void
H17Disk::dumpSectorHeader(unsigned char buf[])
{
    printf("    Sector Header\n");
    printf("       Volume: %3d\n", buf[1]);
    printf("       Track:   %2d\n", buf[2]);
    printf("       Sector:   %d\n", buf[3]);
    printf("       Chksum: 0x%02x\n", buf[4]);
}

void
H17Disk::dumpSectorData(unsigned char buf[])
{
    printf("    Sector Data:\n");
    uint8_t printAble[16];

    for (unsigned int i = 0; i < 256; i++)
    {
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

    printf("       Chksum: %02x\n", buf[256]);
}

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


bool
H17Disk::loadRawDataBlock(unsigned char buf[], unsigned int size)
{
    if (blocks_m[RawDataBlock_c])
    {
        // already a label block specified.
        //
        printf("Duplicate Raw Data Block\n");
    }
    else
    {
        blocks_m[RawDataBlock_c] = new H17DiskRawDataBlock(buf, size);
        printf("New  Raw Data Block - Size: %d\n", size);
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


bool
H17Disk::loadRawTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{
/*    if (blocks_m[SectorDataId])
    {
        // already a label block specified.
        //
        printf("Duplicate TrackBlock\n");
    }
    else
    {
//        blocks_m[TrackBlock_c] = new H17DiskTrackBlock(buf, size);
        printf("New  TrackBlock - Size: %d\n", size);
    } */
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
    dumpDataBlock("Raw Data", &buf[3], blockLength);
#endif

#if 1
    //unsigned char decode[length]; // should be length/2 but don't care about memory right now.
    //unsigned char *buff = new unsigned char[blockLength];
    unsigned char buff[blockLength];
    Decode::decodeFM(buff, &buf[3], blockLength >> 1);

    dumpDataBlock("Data", buff, blockLength >> 1); 

#if 1
    //unsigned char *finalBuff = new unsigned char[(length >> 1) + 5];
    unsigned char finalBuff[blockLength >> 1];

    processSector(buff, finalBuff, blockLength >> 1,  0, 0, 0);
    dumpDataBlock("Aligned Data", finalBuff, blockLength >> 1); 
    //delete[] finalBuff;
#endif
    //delete[] buff;
#endif 
    length = blockLength + 3;
    
    return true;
}

bool
H17Disk::loadRawSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length)
{
/*    if (blocks_m[SectorDataId])
    {
        // already a label block specified.
        //
        printf("Duplicate TrackBlock\n");
    }
    else
    {
//        blocks_m[TrackBlock_c] = new H17DiskTrackBlock(buf, size);
        printf("New  TrackBlock - Size: %d\n", size);
    } */
    return true;
}



// write Header
bool
H17Disk::writeHeader()
{
/*
 *     const uint8_t versionMajor_c = 0;
 *         const uint8_t versionMinor_c = 9;
 *             const uint8_t versionPoint_c = 1;
 *
 *             */
    unsigned char buf[7] = { 'H', '1', '7', 'D', 
                             versionMajor_c, 
                             versionMinor_c,  
                             versionPoint_c };

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

// Common code to write the block header
//
// @param blockId     Block ID
// @param flag        Flag Byte
// @param length      length of block
//
// @returns {bool} if successful
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
    for (int i = 0; i < maxSectors_c; i++)
    {
        curTrackSectors_m[i] = nullptr;
    }
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
                   curTrack_m, i);
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
    // add a new 'rawTrack' to the disk
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
    if( curTrackSectors_m[sector])
    {
        printf("%s: Dup: %d\n", __FUNCTION__, sector);
        
        // unexpected data already for given sector
        return false;
    }

    curTrackSectors_m[sector] = new Sector(curSide_m, curTrack_m, sector, error, buf, length);

    return true;
}


bool
H17Disk::endTrack()
{
    // make sure all sectors are accounted for.
    for (int i = 0; i < maxSectors_c; i++)
    {
        if (!curTrackSectors_m[i])
        {
            // missing data for sector
            printf("Missing data for sector: %d\n", i);
            // fill out an empty block.
            curTrackSectors_m[i] = new Sector(curSide_m, curTrack_m, i,
                                              1 /* TODO - define Err_ReadError*/, 
                                              nullptr, 0);
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

bool
H17Disk::endDataBlock()
{
    // go back and write the size of the data block to the header
    streampos curPos = file_m.tellp();
    uint32_t length = (uint32_t) (curPos - dataBlockSizePos_m - 4);

    unsigned char buf[4] = { (unsigned char) ((length >> 24) & 0xff),
                             (unsigned char) ((length >> 16) & 0xff),
                             (unsigned char) ((length >>  8) & 0xff),
                             (unsigned char) (length         & 0xff) };
    file_m.seekp(dataBlockSizePos_m, ios::beg);
    file_m.write((const char*)buf, 4);
    // go to next byte position after data block.
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
    // header with a zero size
    unsigned char buf[6] = { 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 };
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
                              (unsigned char) (length         & 0xff) };
    // position to the size field
    file_m.seekp(rawSizePos, ios::beg);
    file_m.write((const char*)buf2, 4);
    // position to next byte after this block.
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
    // allocate a new raw sector, and store it to the current track.
    RawSector *tmp = new RawSector(curSide_m, curTrack_m, sector, buf, length); 
    rawTracks_m[curTrack_m]->addRawSector(tmp);
    return true;
}

char
*H17Disk::getSectorData(unsigned char side,
                        unsigned char track,
                        unsigned char sector)
{

    return 0;
}


bool
H17Disk::setDefaults()
{
    bool retVal = true;

    if (!setDefaultDiskFormat())
    {
        retVal = false;
    }

    if (!setDefaultFlags())
    {
        retVal = false;
    }

    return retVal;
}

bool
H17Disk::setDefaultDiskFormat()
{
    sides_m = 1;
    tracks_m = 40;

    return true;
}

bool
H17Disk::setDefaultFlags()
{
    distribution_m = DistUnknown;
    trackDataSource_m = TrackDataUnknown;
    writeProtect_m = false;

    return true;
}

bool
H17Disk::readHeader()
{

    return false;
}

bool
H17Disk::readBlocks()
{

    return false;
}

bool
H17Disk::readDiskFormatBlock()
{

    return false;
}

bool
H17Disk::readFlagsBlock()
{

    return false;
}

bool
H17Disk::readLabelBlock()
{

    return false;
}

bool
H17Disk::readCommentBlock()
{

    return false;
}

bool
H17Disk::readDateBlock()
{

    return false;
}

bool
H17Disk::readImagerBlock()
{

    return false;
}

bool
H17Disk::readProgramBlock()
{

    return false;
}

bool
H17Disk::readDataBlock()
{

    return false;
}

bool
H17Disk::readRawDataBlock()
{

    return false;
}

