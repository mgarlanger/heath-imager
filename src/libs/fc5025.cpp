//!  \file fc5025.cpp
//!
//! Communicate with the FC5025 hardware
//!

#include "fc5025.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <usb.h>

#define swap32(x) (((((uint32_t)x) & 0xff000000) >> 24) | \
                   ((((uint32_t)x) & 0x00ff0000) >>  8) | \
                   ((((uint32_t)x) & 0x0000ff00) <<  8) | \
                   ((((uint32_t)x) & 0x000000ff) << 24))

#define htov32(x) swap32(htonl(x))

FC5025 *FC5025::pInst_m = nullptr;

// \todo move this into class.
//static usb_dev_handle *udev;

// move this too.
static struct
{
    uint8_t      signature[4];
    uint32_t     tag,
                 xferlen;
    uint8_t      flags;
    uint8_t      padding1,
                 padding2;
    uint8_t      cdb[48];
} __attribute__ ((__packed__)) cbw =
{
    { 'C', 'F', 'B', 'C'}, // signature
    0x12345678,            // tag
    0,                     // xferlen
    0x80,                  // flags
    0,                     // padding1 
    0,                     // padding2
    { 0,}                  // cdb[]
};


//! Constructor
//!
FC5025::FC5025()
{
    // set drive parameters to default
    // - default for TEAC  1.2M
    drive_Tracks_m   = 80;
    drive_Sides_m    = 2;
    drive_RPM_m      = 360;
    drive_StepRate_m = 15;
   
    // H-17-1 
    //drive_Tracks_m   = 40;
    //drive_Sides_m    = 1;
    //drive_RPM_m      = 300;
    //drive_StepRate_m = 150;   // 30 mSec

    // H-17-4 
    //drive_Tracks_m   = 80;
    //drive_Sides_m    = 2;
    //drive_RPM_m      = 300;
    //drive_StepRate_m = 30;   // 6 mSec

    // default hard-sectored disk
    disk_Tracks_m    = 40;
    disk_Sides_m     = 1;
    disk_RPM_m       = 300;
}


//! Destructor
//!
FC5025::~FC5025()
{

}


//! get singleton
//!
//! @return FC5025
//!
FC5025 *
FC5025::inst()
{
    if (pInst_m == nullptr)
    {
        pInst_m = new FC5025();
        usb_init();
    }

    return pInst_m;
}


//! common cdb 
//!
//! @param cdb
//! @param length
//! @param timeout
//! @param csw_out
//! @param xferbuf
//! @param xferlen
//! @param xferlen_out
//!
//! @return status
//!
int
FC5025::bulkCDB(void          *cdb, 
                int            length,
                int            timeout,
                uint8_t       *csw_out,
                uint8_t       *xferbuf,
                int            xferlen,
                int           *xferlen_out)
{
    // response structure
    struct
    {
        uint32_t   signature;
        uint32_t   tag;
        uint8_t    status;
        uint8_t    sense;
        uint8_t    asc;
        uint8_t    ascq;
        uint8_t    padding[20];
    } __attribute__ ((__packed__)) csw;

    int             ret;

    cbw.tag++;
    cbw.xferlen = htov32(xferlen);

    memset(&(cbw.cdb), 0, 48);
    memcpy(&(cbw.cdb), cdb, length);

    if (xferlen_out != NULL)
    {
        *xferlen_out = 0;
    }

    ret = usb_bulk_write(udev_m, 1, (const char *) &cbw, 63, 1500);
    if (ret != 63)
    {
        printf("%s: failed 1\n", __FUNCTION__);
        return 1;
    }

    // get data if requested, on error FC5025 will send a zero length response
    // followed by the status
    if (xferlen != 0)
    {
        ret = usb_bulk_read(udev_m, 0x81, (char *) xferbuf, xferlen, timeout);
        if (ret < 0)
        {
            printf("%s: failed 2\n", __FUNCTION__);
            return 1;
        }
        if (xferlen_out != NULL)
        {
            *xferlen_out = ret;
        }
        timeout = 500;
    }

    // get the status
    ret = usb_bulk_read(udev_m, 0x81, (char *) &csw, 32, timeout);
    if ((ret < 12) || (ret > 31))
    {
        printf("%s: failed 3\n", __FUNCTION__);
        return 1;
    }

    if (csw.signature != htov32(cswSignature_c))
    {
        printf("%s: failed 4\n", __FUNCTION__);
        return 1;
    }

    // verify tag
    if (csw.tag != cbw.tag)
    {
        // response tag did not match transmitted tag
        printf("%s: failed 5\n", __FUNCTION__);
        return 1;
    }

    // verify buffer was provided
    if (csw_out != NULL)
    {
        memcpy(csw_out, &csw, 12);
    }

    if (csw.status)
    {
        printf("failure status - Key: %02x  ASC: %02x  ASCQ: %02x\n", csw.sense,
                 csw.asc, csw.ascq);
        lastSenseKey_m = csw.sense;
        lastASC_m      = csw.asc;
        lastASCQ_m     = csw.ascq;
    }

    return(csw.status);
}


//! internal seek command
//!
//! @param mode
//! @param stepRate
//! @param track
//! 
//! @return status
//!
int
FC5025::internalSeek(uint8_t mode,
                     uint8_t stepRate,
                     uint8_t track)
{
    int retVal;

    struct
    {   
        Opcode    opcode;
        uint8_t   mode;
        uint8_t   steprate;
        uint8_t   track;
    } __attribute__ ((__packed__)) cdb =
    {   
        Opcode::Seek,
        mode,
        stepRate,
        track
    };
    
    retVal = bulkCDB(&cdb, sizeof(cdb), 600, NULL, NULL, 0, NULL);
    usleep(15000);

    return retVal;
}


//! recalibrate - try to find track zero
//! 
//! @return status
//!
int
FC5025::recalibrate(void)
{
    return internalSeek( 3, drive_StepRate_m, 100);
}


//! seek to a specified track
//!
//! @param track
//!
//! @return status
//!
int
FC5025::seek(uint8_t       track)
{
    return internalSeek(0, drive_StepRate_m, track);
}


//! read id 
//!
//! @param out
//! @param length
//! @param side
//! @param format
//! @param bitcell
//! @param idam0
//! @param idam1
//! @param idam2
//!
//! @return status
//!
int
FC5025::readId(uint8_t       *out,
               int            length,
               uint8_t        side,
               uint8_t        format,
               int            bitcell,
               uint8_t        idam0,
               uint8_t        idam1,
               uint8_t        idam2)
{
    struct
    {
        Opcode     opcode;
        uint8_t    side;
        uint8_t    format;
        uint16_t   bitcell;
        uint8_t    idam0;
        uint8_t    idam1;
        uint8_t    idam2;
    } __attribute__ ((__packed__)) cdb =
    {
        Opcode::ReadId,
        side,
        format,
        htons(bitcell),
        idam0,
        idam1,
        idam2
    };
    int     xferlen_out;
    int     status = 0;

    status = bulkCDB(&cdb, sizeof(cdb), 3000, NULL, out, length, &xferlen_out);

    if (xferlen_out != length)
    {
        status |= 1;
    }

    return status;
}


//! set flags
//!
//! @param in
//! @param mask
//! @param out
//!
//! @return status
//!
int
FC5025::flags(uint8_t        in,
              uint8_t        mask,
              int           *out)
{
    struct
    {
        Opcode     opcode;
        uint8_t    mask;
        uint8_t    flags;
    } __attribute__ ((__packed__)) cdb =
    {
        Opcode::Flags,
        mask,
        in
    };
    uint8_t         buf;
    int             ret;
    int             xferlen_out;

    ret = bulkCDB(&cdb, sizeof(cdb), 1500, NULL, &buf, 1, &xferlen_out);

    if (xferlen_out == 1)
    {
        if (out != NULL)
        {
            *out = buf;
        }
        return ret;
    }

    return 1;
}


//! get drive status
//!
//! @param track
//! @param diskSpeed
//! @param sectorCount
//! @param ds_flags
//!
//! @return status
//!
int
FC5025::driveStatus(uint8_t  *track,
                    uint16_t *diskSpeed,
                    uint8_t  *sectorCount,
                    uint8_t  *ds_flags)
{
    int status = 0;

    // first turn on motor
    if ((status = flags(0x01, 0x01, NULL)) != 0)
    {
        return status;
    }

    // Then read status
    struct
    {
        Opcode          opcode;
    } __attribute__ ((__packed__)) cdb =
    {
        Opcode::DriveStatus
    }; 
    uint8_t       buf[5];
    int           xferlen_out;

    status = bulkCDB(&cdb, sizeof(cdb), 100, NULL, buf, 5, &xferlen_out);

    if ((!status) && (xferlen_out == 5))
    {
        *track       = buf[0];
        *diskSpeed   = buf[1] << 8 | buf[2];
        *sectorCount = buf[3];
        *ds_flags    = buf[4];
    }
 
    // need to turn off motor and preserve the error status
    status |= flags(0x00, 0x01, NULL);
    
    return status;
}


//! set density
//!
//! @param density
//!
//! @return status
//!
int
FC5025::setDensity(int density)
{
    return flags(density << 2, 0x04, NULL);
}


//! open device
//!
//! @param dev  device to open
//!
//! @returns 0 - success, 1 - failure
//!
int
FC5025::open(struct usb_device *dev)
{
    cbw.tag = time(NULL) & 0xffffffff;
    udev_m    = usb_open(dev);

    if (!udev_m)
    {
        return 1;
    }

    if (usb_claim_interface(udev_m, 0) != 0)
    {
        usb_close(udev_m);
        return 1;
    }

    return 0;
}


//! close device
//!
//! @return status
//!
int
FC5025::close(void)
{
    if ((usb_release_interface(udev_m, 0) != 0) || (usb_close(udev_m) != 0))
    {
        return 1;
    }

    return 0;
}


//! find devices
//!
//! @param devs   list of the devices found
//! @param max    maximum number of devices to return
//!
//! @returns number of devices found
//!
int
FC5025::find(struct usb_device **devs, 
             int                 max)
{
    struct usb_bus    *bus;
    struct usb_device *dev;
    int                num_found = 0;

    usb_find_busses();
    usb_find_devices();

    for (bus = usb_get_busses(); bus != NULL; bus = bus->next)
    {
        for (dev = bus->devices; dev != NULL; dev = dev->next)
        {
            if ((dev->descriptor.idVendor == VendorID_c) && 
                (dev->descriptor.idProduct == ProductID_c))
            {
                num_found++;
                if (num_found <= max)
                {
                    *(devs++) = dev;
                }
            }
        }
    }

    return num_found;
}


//! configure disk drive
//!
//! @param tracks
//! @param sides
//! @param rpm
//! @param stepRate
//!
void
FC5025::configureDiskDrive(uint8_t   tracks,
                           uint8_t   sides,
                           uint16_t  rpm,
                           uint8_t   stepRate)
{
    drive_Tracks_m   = tracks;
    drive_Sides_m    = sides;
    drive_RPM_m      = rpm;
    drive_StepRate_m = stepRate;
}


//! get disk drive
//!
//! @param tracks
//! @param sides
//! @param rpm
//! @param stepRate
//!
void 
FC5025::getDiskDrive(uint8_t   &tracks,
                     uint8_t   &sides,
                     uint16_t  &rpm,
                     uint8_t   &stepRate)
{
    tracks   = drive_Tracks_m;
    sides    = drive_Sides_m;
    rpm      = drive_RPM_m;
    stepRate = drive_StepRate_m;
}


//! configure floppy disk
//!
//! @param tracks
//! @param sides
//! @param rpm
//!
void
FC5025::configureFloppyDisk(uint8_t  tracks,
                            uint8_t  sides,
                            uint16_t rpm)
{
    disk_Tracks_m = tracks;
    disk_Sides_m  = sides;
    disk_RPM_m    = rpm;
}

