#ifndef __DRIVE_H__
#define __DRIVE_H__

#include <stdint.h>
#include <usb.h>


// store info about the FC5025 devices
struct DriveInfo
{
    char               id[256];
    char               desc[256];
    struct usb_device *usbdev;
};


// 
class Drive
{
public:
    Drive(DriveInfo *drive);
    ~Drive();

    uint8_t getStatus();
    static DriveInfo *get_drive_list (void);

private:
    uint8_t status_m;
};

#endif

