#ifndef __FC5025_H__
#define __FC5025_H__

#include <usb.h>
#include <cstdint>

class FC5025 
{
public:

    static FC5025* inst();

    int bulkCDB(void            *cdb,
                int              length,
                int              timeout,
                unsigned char   *csw_out,
                unsigned char   *xferbuf,
                int              xferlen,
                int             *xferlen_out);

    int recalibrate(void);

    int seek(unsigned char       track);

    int readId(unsigned char    *out,
               int               length,
               unsigned char     side,
               unsigned char     format,
               int               bitcell,
               unsigned char     idam0,
               unsigned char     idam1,
               unsigned char     idam2);

    int flags(unsigned char      in,
              unsigned char      mask,
              int               *out);

    int setDensity(int           density);

    int open(struct usb_device  *dev);

    int close(void);

    int find(struct usb_device **devs, 
              int                max);

    int driveStatus(uint8_t     *track,
                    uint16_t    *speed,
                    uint8_t     *sectorCount,
                    uint8_t     *flags);


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

    static const uint8_t ReadFlag_Side_c             = 0x01;
    static const uint8_t ReadFlag_IdField_c          = 0x02;
    static const uint8_t ReadFlag_OverrunRecovery_c  = 0x04;
    static const uint8_t ReadFlag_NoAutosync_c       = 0x08;
    static const uint8_t ReadFlag_Angular_c          = 0x10;
    static const uint8_t ReadFlag_NoAdaptive_c       = 0x20;

private:

    FC5025();
    virtual ~FC5025();

    static FC5025 *pInst_m;

    static const uint16_t VendorID  = 0x16c0;
    static const uint16_t ProductID = 0x06d6;

    int internalSeek(uint8_t mode,
                     uint8_t stepRate,
                     uint8_t track);
};

#endif

