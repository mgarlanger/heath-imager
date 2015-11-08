//! \file drive.h
//!
//! Physical Drive control and configuration
//!

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


    bool setHeads(uint8_t heads);
    bool setTpi(uint8_t tpi);
    bool setRpm(uint16_t rpm);

    uint8_t getHeads() { return heads_m; }
    uint8_t getTpi()   { return tpi_m;   }
    uint16_t getRpm()  { return rpm_m;   } 

private:
    uint8_t status_m;

    uint8_t heads_m;
    uint8_t tpi_m;
    uint16_t rpm_m;

};


#endif

