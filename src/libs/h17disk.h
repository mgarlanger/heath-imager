//! \file h17disk.h
//!
//! Handles the h17disk file format.
//!

#ifndef __H17DISK_H__
#define __H17DISK_H__

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

class H17Block;
class RawTrack;
class Sector;

class H17Disk
{
public:
    H17Disk();
    virtual ~H17Disk();

    // Block IDs
    //
    static const uint8_t DiskFormatBlock_c;
    static const uint8_t FlagsBlock_c;
    static const uint8_t LabelBlock_c;
    static const uint8_t CommentBlock_c;
    static const uint8_t DateBlock_c;
    static const uint8_t ImagerBlock_c;
    static const uint8_t ProgramBlock_c;
   
    static const uint8_t DataBlock_c;
    static const uint8_t RawDataBlock_c;

    // SubBlock IDs
    //
    static const uint8_t TrackDataId;
    static const uint8_t SectorDataId;

    static const uint8_t RawTrackDataId;
    static const uint8_t RawSectorDataId;

    // flags
    static const uint8_t DistUnknown;
    static const uint8_t DistributionDisk;
    static const uint8_t WorkingDisk;
 
    //  
    static const uint8_t TrackDataUnknown;
    static const uint8_t TrackDataGeneratedFromH8dConversion;
    static const uint8_t TrackDataCreatedOnEmulator;
    static const uint8_t TrackDataCapturedOnH89;
    static const uint8_t TrackDataCapturedOnFC5025;

    static const uint8_t MandatoryFlagMask;
    static const uint8_t MandatoryFlag_Mandatory;
    static const uint8_t MandatoryFlag_NotMandatory;


    // Open a file
    virtual bool openForWrite(char *name);
    virtual bool openForRead(char *name);
    virtual bool openForRecovery(char *name);

    virtual bool loadFile();
    virtual bool saveFile(char  *name);
    virtual bool saveAsH8D(char *name);
    virtual bool saveAsRaw(char *name);

    virtual bool loadBuffer(unsigned char buf[], unsigned int size);
    virtual bool loadHeader(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool loadBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool loadDiskFormatBlock(unsigned char buf[], unsigned int size);
    virtual bool loadFlagsBlock(unsigned char buf[], unsigned int size);
    virtual bool loadLabelBlock(unsigned char buf[], unsigned int size);
    virtual bool loadCommentBlock(unsigned char buf[], unsigned int size);
    virtual bool loadDateBlock(unsigned char buf[], unsigned int size);
    virtual bool loadImagerBlock(unsigned char buf[], unsigned int size);
    virtual bool loadProgramBlock(unsigned char buf[], unsigned int size);
    virtual bool loadDataBlock(unsigned char buf[], unsigned int size);
    virtual bool loadTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool loadSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool loadRawDataBlock(unsigned char buf[], unsigned int size);
    virtual bool loadRawTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool loadRawSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length);

    virtual bool analyze();
    virtual bool decodeFile(char *name);
    virtual bool reprocessFile();

    virtual bool decodeBuffer(unsigned char buf[], unsigned int size);
    virtual bool validateHeader(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool validateBlock(unsigned char buf[], unsigned int size, unsigned int &length);

    virtual bool validateDiskFormatBlock(unsigned char buf[], unsigned int size);
    virtual bool validateFlagsBlock(unsigned char buf[], unsigned int size);

    virtual bool validateLabelBlock(unsigned char buf[], unsigned int size);
    virtual bool validateCommentBlock(unsigned char buf[], unsigned int size);
    virtual bool validateDateBlock(unsigned char buf[], unsigned int size);
    virtual bool validateImagerBlock(unsigned char buf[], unsigned int size);
    virtual bool validateProgramBlock(unsigned char buf[], unsigned int size);

    virtual bool validateDataBlock(unsigned char buf[], unsigned int size);
    virtual bool validateTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool validateSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool validateRawDataBlock(unsigned char buf[], unsigned int size);
    virtual bool validateRawTrackBlock(unsigned char buf[], unsigned int size, unsigned int &length);
    virtual bool validateRawSectorBlock(unsigned char buf[], unsigned int size, unsigned int &length);

    virtual void dumpSectorHeader(unsigned char buf[]);
    virtual void dumpSectorData(unsigned char buf[]);

    virtual bool closeFile(void);

    virtual void disableRaw();

    // write Header
    virtual bool writeHeader();

    virtual bool setSides(unsigned char sides);
    virtual bool setTracks(unsigned char tracks);

    virtual bool writeDiskFormatBlock();
  
    virtual bool setWPParameter(bool val); 
    virtual bool setDistributionParameter(unsigned char val);
    virtual bool setTrackDataParameter(unsigned char val);

    virtual bool writeLabel(unsigned char *buf, uint32_t length);
    virtual bool writeComment(unsigned char *buf, uint32_t length);
    virtual bool writeDate(unsigned char *buf, uint32_t length);
    virtual bool writeImager(unsigned char *buf, uint32_t length);
    virtual bool writeProgram(unsigned char *buf, uint32_t length);

    virtual bool writeParameters();

    virtual bool startData();

    virtual bool startTrack(unsigned char side,
                           unsigned char track);
    virtual bool addSector(unsigned char sector,
                           unsigned char error,
                           unsigned char *buf,
                           uint16_t      length);
    virtual bool addRawSector(unsigned char sector, 
                              unsigned char *buf,
                              uint16_t      length);

    virtual bool endTrack();
    virtual bool endDataBlock();

    virtual bool writeRawDataBlock();

    virtual char *getSectorData(unsigned char side,
                                unsigned char track,
                                unsigned char sector);

    virtual bool addSectorToDataBlock(uint8_t   side,
                                      uint8_t   track,
                                      uint8_t   sector,
                                      uint8_t  *buf,
                                      uint16_t  length);

    virtual bool addRawSectorToDataBlock(uint8_t   side,
                                         uint8_t   track,
                                         uint8_t   sector,
                                         uint8_t  *buf,
                                         uint16_t  length);

//  - raw data...    virtual bool convertToData();

    static const uint8_t versionMajor_c;
    static const uint8_t versionMinor_c;
    static const uint8_t versionPoint_c;

private:

    unsigned char sides_m;
    unsigned char tracks_m;
    
    unsigned char curSide_m;
    unsigned char curTrack_m;

    unsigned char distribution_m;
    unsigned char trackDataSource_m;
    bool          writeProtect_m;
    bool          disableRaw_m;

    uint8_t       versionMajor_m;
    uint8_t       versionMinor_m;
    uint8_t       versionPoint_m;

    H17Block     *blocks_m[256];

    std::ifstream inFile_m; 
    std::ofstream file_m; 

    unsigned int  dataSize_m;

    std::streampos  dataBlockSizePos_m, trackSizePos_m;

//    virtual bool writeHeader();

    bool writeBlockHeader(uint8_t blockId, uint8_t flag, uint32_t length);

//    static const unsigned char maxTracks_c  = 160; // 2 sided, 80 tracks
    static const unsigned char maxSectors_c = 10;

    std::vector<RawTrack *> rawTracks_m;
    Sector *curTrackSectors_m[maxSectors_c];

    unsigned int sectorErrs_m;

    //std::vector<Track *> tracksData_m;

    virtual bool setDefaults();
    virtual bool setDefaultDiskFormat();
    virtual bool setDefaultFlags();

    static const uint8_t defaultSides_c  = 1;
    static const uint8_t defaultTracks_c = 40;
 
};

#endif

