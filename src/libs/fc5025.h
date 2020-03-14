//! \file fc5025.h
//!
//! Communication with the FC5025 hardware
//!

#ifndef __FC5025_H__
#define __FC5025_H__

#include <cstdint>

struct usb_device;
struct usb_dev_handle;


class FC5025 
{

public:

    static FC5025* inst();

    int bulkCDB(void                 *cdb,
                int                   length,
                int                   timeout,
                uint8_t              *csw_out,
                uint8_t              *xferbuf,
                int                   xferlen,
                int                  *xferlen_out);

    int recalibrate(void);

    int seek(uint8_t                  track);

    int readId(uint8_t               *out,
               int                    length,
               uint8_t                side,
               uint8_t                format,
               int                    bitcell,
               uint8_t                idam0,
               uint8_t                idam1,
               uint8_t                idam2);

    int readHardSectorSector(
               uint8_t               *out,
               uint16_t               length,
               uint8_t                side,
               uint8_t                track,
               uint8_t                sector,
               uint16_t               bitcellTime);

    int flags(uint8_t                 in,
              uint8_t                 mask,
              int                    *out);

    int testBoard(                    );

    int setDensity(int                density);

    int open(struct usb_device       *dev);

    int close(void);

    int find(struct usb_device      **devs, 
              int                     max);

    int driveStatus(uint8_t          *track,
                    uint16_t         *speed,
                    uint8_t          *sectorCount,
                    uint8_t          *flags);

    void setStepRate(uint8_t         stepRate);

    void getStepRate(uint8_t        &stepRate);

    enum class Opcode : uint8_t
    {
        Seek         = 0xc0,
        SelfTest     = 0xc1,
        Flags        = 0xc2,
        DriveStatus  = 0xc3,
        Indexes      = 0xc4,
        ReadFlexible = 0xc6,
        ReadId       = 0xc7
    };

    enum class Format : uint8_t
    {
       AppleGCR      = 1,
       CommodoreGCR  = 2,
       FM            = 3,
       MFM           = 4
    };

    enum class Density: uint8_t
    {
        // TODO determine the proper values
        //Single       = 0;
        //Double       = 1;
    };

    static const uint8_t ReadFlag_Side_c             = 0x01;
    static const uint8_t ReadFlag_IdField_c          = 0x02;
    static const uint8_t ReadFlag_OverrunRecovery_c  = 0x04;
    static const uint8_t ReadFlag_NoAutosync_c       = 0x08;
    static const uint8_t ReadFlag_Angular_c          = 0x10;
    static const uint8_t ReadFlag_NoAdaptive_c       = 0x20;

    enum class Key : uint8_t 
    {
        NoError      = 0,
        SeekError    = 2,
        DiagError    = 4,
        CommandError = 5
    };

    enum class ASC_Seek : uint8_t
    {
        NoReferencePosition = 6,
    };

    enum class ASC_Diag : uint8_t 
    {
        DiagnoticFailure = 0x40,
    };
 
    enum class ASCQ_Diag : uint8_t 
    {
        DiagnoticFailure = 0x80,
    };
 
    enum class ASC_Command : uint8_t
    {
        InvalidCommand = 0x20,
        InvalidField   = 0x24,
        SequenceError  = 0x2c,
    };


    static const uint8_t testResponseSize_c;

private:

    struct CommandBlockWrapper
    {
        uint8_t      signature[4];
        uint32_t     tag,
                     xferlen;
        uint8_t      flags;
        uint8_t      padding1,
                     padding2;
        uint8_t      cdb[48];
    } __attribute__ ((__packed__));

    FC5025();
    virtual ~FC5025();
    static FC5025 *pInst_m;

    static const uint16_t VendorID_c  = 0x16c0;
    static const uint16_t ProductID_c = 0x06d6;

    static const uint32_t cswSignature_c = 0x46435342;

    int internalSeek(uint8_t mode,
                     uint8_t track);

    usb_dev_handle *udev_m;
    uint8_t  lastSenseKey_m;
    uint8_t  lastASC_m;
    uint8_t  lastASCQ_m;

    uint8_t  drive_StepRate_m;
//    CommandBlockWrapper *cbw;

};

#endif

