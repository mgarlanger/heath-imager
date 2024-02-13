//! \file decode.h
//!
//!  Decode FM and MFM encoded bit streams.
//!

#ifndef __DECODE_H__
#define __DECODE_H__

#include <stdint.h>

class Decode
{
public:

    static int decodeFM(uint8_t      *decoded,
                        uint8_t      *fmEncoded, 
                        unsigned int  count);

    static int decodeMFM(uint8_t      *decoded,
                         uint8_t      *mfmEncoded,
                         unsigned int  count);

private:

    //! current state of the decoding
    enum State { 
       none,  // Haven't yet determined the position of the data bit
       hi,    // Expect data bit to be in the high bit
       lo     // Expect data bit to be in the low bit
    };

    static int lastZeroErrors;
    static int lastOneErrors;

};

#endif
