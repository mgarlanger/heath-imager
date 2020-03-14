//! \file drive.cpp
//!
//!  Implementation of physical drive control
//!

#include "drive.h"
#include "fc5025.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static DriveInfo *drives = NULL;


//! constructor
//!
Drive::Drive(DriveInfo *drive):heads_m(2),
                               tpi_m(96),
                               rpm_m(360)
{
    status_m = FC5025::inst()->open(drive->usbdev);
}


//! destructor
//!
Drive::~Drive()
{
    FC5025::inst()->close();
}


//! get status
//!
//! @return staus
//!
uint8_t
Drive::getStatus()
{
    return status_m;
}


//! get description
//!
//! @param drive 
//!
//! @return  status with 0 equals success
//!
static int
get_desc(DriveInfo *drive)
{
    struct usb_device_descriptor  descriptor;
    usb_dev_handle               *udev;
    int                           retVal = 0;

    udev = usb_open(drive->usbdev);
    if (!udev)
    {
        return 1;
    }

    if (usb_get_descriptor(udev, USB_DT_DEVICE, 0, &descriptor, sizeof(descriptor)) != 
                           sizeof(descriptor) ||
       (usb_get_string_simple(udev, descriptor.iProduct, drive->desc, 256) <= 0))
    {
        retVal = 1;
    }

    usb_close(udev);
    return retVal;
}


//! get drive list
//!
//! @return drive info
//!
DriveInfo *
Drive::get_drive_list(void)
{
    int                        total_devs,
                               listed_devs;
    static struct usb_device **devs = NULL;
    int                        i;
    struct usb_device        **dev;
    DriveInfo                 *drive;

    total_devs = FC5025::inst()->find(NULL, 0);

    if (total_devs == 0)
    {
        return NULL;
    }

    if (devs != NULL)
    {
        free(devs);
    }

    devs = (struct usb_device **) malloc(total_devs * sizeof(struct usb_device));

    if (!devs)
    {
        return NULL;
    }

    listed_devs = FC5025::inst()->find(devs, total_devs);

    if (listed_devs == 0)
    {
        return NULL;
    }

    if (drives != NULL)
    {
        free(drives);
    }

    drives = (DriveInfo *) malloc((listed_devs + 1) * sizeof(DriveInfo));

    if (!drives)
    {
        return NULL;
    }

    dev = devs;
    drive = drives;

    for (i = 0; i < listed_devs; i++)
    {
        snprintf(drive->id, 256, "%s/%s", (*dev)->bus->dirname, (*dev)->filename);
        drive->usbdev = *dev;
        if (get_desc(drive) == 0)
        {
            dev++;
            drive++;
        }
    }

    drive->id[0]   = '\0';
    drive->desc[0] = '\0';
    drive->usbdev  = NULL;

    if (drive == drives)
    {
        return NULL;
    }

    return drives;
}


//! set number of heads for the drive 
//!
//! @param heads
//!
//! @return success
//!
bool
Drive::setHeads(uint8_t heads)
{
    bool status = false;

    if ((heads == 1) || (heads == 2))
    {
        heads_m = heads;
        status = true;
    }

    return status;
}


//! set tpi for the drive
//!
//! @param tpi
//!
//! @return success
//!
bool
Drive::setTpi(uint8_t tpi)
{
    bool status = false;

    if ((tpi == 48) || (tpi == 96))
    {
        tpi_m = tpi;
        status = true;
    }

    return status;
}


//! set RPM of the drive
//! 
//! @param rpm
//!
//! return success
//!
bool
Drive::setRpm(uint16_t rpm)
{
    bool status = false;

    if ((rpm == 300) || (rpm == 360))
    {
        rpm_m = rpm;
        status = true;
    }

    return status;
}

