#include "drive.h"
#include "fc5025.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <usb.h>

static DriveInfo *drives = NULL;

Drive::Drive(DriveInfo *drive)
{
    status_m = FC5025::inst()->open(drive->usbdev);
}

uint8_t
Drive::getStatus()
{
    return status_m;
}

Drive::~Drive()
{
    FC5025::inst()->close();
}

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
                           sizeof(descriptor))
    {
        retVal = 1;
        goto done;
    }
    if (usb_get_string_simple(udev, descriptor.iProduct, drive->desc, 256) <= 0)
    {
        retVal = 1;
        goto done;
    }

done:
    usb_close(udev);
    return retVal;
}

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

