//! \file decode.h
//!
//!  Decode FM and MFM encoded bit streams.
//!

#ifndef __DECODE_H__
#define __DECODE_H__

#include "hi_types.h"


class Decode
{
public:

    static int decodeFM(BYTE         *decoded,
                        BYTE         *fmEncoded, 
                        unsigned int  count);

    static int decodeMFM(BYTE         *decoded,
                         BYTE         *mfmEncoded,
                         unsigned int  count);

private:

    //! current state of the decoding
    enum State { 
       none,  // Haven't yet determined the position of the data bit
       hi,    // Expect data bit to be in the high bit
       lo     // Expect data bit to be in the low bit
    };
};

#endif

