#ifndef __DECODE_H__
#define __DECODE_H__

#include "hi_types.h"

class Decode
{
public:
    static int decodeFM(BYTE *decoded, BYTE *fmEncoded, int count);
    static int decodeMFM(BYTE *decoded, BYTE *fmEncoded, int count);
};


#endif

