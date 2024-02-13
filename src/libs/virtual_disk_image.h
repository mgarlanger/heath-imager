//! \file virtual_disk_image.h
//!
//! Parent class for all virtual disk image formats, such as h17disk, h17raw, h8d, td0, imd..
//!

#ifndef __VIRTUAL_DISK_IMAGE__
#define __VIRTUAL_DISK_IMAGE__

#include <stdint.h>

class VirtualDiskImage
{
  public:
    VirtualDiskImage();
    virtual ~VirtualDiskImage();


    virtual uint8_t getNumberHeads() = 0;
    virtual uint8_t getNumberTracks() = 0;
    virtual uint8_t getNumberSectors() = 0;

    virtual uint8_t getSectorStatus(uint8_t side,
                                    uint8_t track,
                                    uint8_t sector) = 0;

    virtual char *getSectorData(uint8_t side,
                                uint8_t track,
                                uint8_t sector) = 0;

  private:

};

#endif
