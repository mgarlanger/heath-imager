/* Communicate with the FC5025 hardware */
#include "fc5025.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#define swap32(x) (((((uint32_t)x) & 0xff000000) >> 24) | \
                   ((((uint32_t)x) & 0x00ff0000) >>  8) | \
                   ((((uint32_t)x) & 0x0000ff00) <<  8) | \
                   ((((uint32_t)x) & 0x000000ff) << 24))

#define htov32(x) swap32(htonl(x))

FC5025 *FC5025::pInst_m = nullptr;

// \TODO move this into class.
static usb_dev_handle *udev;

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

FC5025::FC5025()
{
    // set disk parameters to default
    // - default for TEAC 
    drive_Tracks_m   = 80;
    drive_Sides_m    = 2;
    drive_RPM_m      = 360;
    drive_StepRate_m = 15;

    // default hard-sectored disk
    disk_Tracks_m    = 40;
    disk_Sides_m     = 1;
    disk_RPM_m       = 300;

}

FC5025::~FC5025()
{

}

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

int
FC5025::bulkCDB(void          *cdb, 
                int            length,
                int            timeout,
                unsigned char *csw_out,
                unsigned char *xferbuf,
                int            xferlen,
                int           *xferlen_out)
{
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

    ret = usb_bulk_write(udev, 1, (char *) &cbw, 63, 1500);
    if (ret != 63)
    {
        printf("%s: failed 1\n", __FUNCTION__);
        return 1;
    }

    if (xferlen != 0)
    {
        ret = usb_bulk_read(udev, 0x81, (char *) xferbuf, xferlen, timeout);
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

    ret = usb_bulk_read(udev, 0x81, (char *) &csw, 32, timeout);
    if ((ret < 12) || (ret > 31))
    {
        printf("%s: failed 3\n", __FUNCTION__);
        return 1;
    }

    if (csw.signature != htov32(0x46435342))
    {
        printf("%s: failed 4\n", __FUNCTION__);
        return 1;
    }
    if (csw.tag != cbw.tag)
    {
        printf("%s: failed 5\n", __FUNCTION__);
        return 1;
    }

    if (csw_out != NULL)
    {
        memcpy(csw_out, &csw, 12);
    }

    return(csw.status);
}

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

int
FC5025::recalibrate(void)
{
    return internalSeek( 3, drive_StepRate_m, 100);
}

int
FC5025::seek(unsigned char track)
{
    return internalSeek(0, drive_StepRate_m, track);
}

int
FC5025::readId(unsigned char *out,
               int            length,
               unsigned char  side,
               unsigned char  format,
               int            bitcell,
               unsigned char  idam0,
               unsigned char  idam1,
               unsigned char  idam2)
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

int
FC5025::flags(unsigned char  in,
              unsigned char  mask,
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
    unsigned char   buf;
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
    unsigned char buf[5];
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

int
FC5025::setDensity(int density)
{
    return flags(density << 2, 0x04, NULL);
}


// open()
//
// @param dev  device to open
//
// @returns 0 - success, 1 - failure
int
FC5025::open(struct usb_device *dev)
{
    cbw.tag = time(NULL) & 0xffffffff;
    udev    = usb_open(dev);

    if (!udev)
    {
        return 1;
    }

    if (usb_claim_interface(udev, 0) != 0)
    {
        usb_close(udev);
        return 1;
    }

    return 0;
}

int
FC5025::close(void)
{
    if ((usb_release_interface(udev, 0) != 0) || (usb_close(udev) != 0))
    {
        return 1;
    }

    return 0;
}


// find()
//
// @param devs   list of the devices found
// @param max    maximum number of devices to return
//
// @returns number of devices found
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
            if ((dev->descriptor.idVendor == VendorID) && 
                (dev->descriptor.idProduct == ProductID))
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

void
FC5025::configureFloppyDisk(uint8_t  tracks,
                            uint8_t  sides,
                            uint16_t rpm)
{
    disk_Tracks_m = tracks;
    disk_Sides_m  = sides;
    disk_RPM_m    = rpm;
}

