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
                         BYTE         *fmEncoded,
                         unsigned int  count);

private:
    enum State { none, hi, lo };
};

#endif

