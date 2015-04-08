#ifndef __DECODE_H__
#define __DECODE_H__

#include "hi_types.h"

class Decode
{
private:
    enum State { none, hi, lo };

public:
    static int decodeFM(BYTE *decoded, BYTE *fmEncoded, unsigned int count);
    static int decodeFM_old(BYTE *decoded, BYTE *fmEncoded, unsigned int count);
    static int decodeFM_old2(BYTE *decoded, BYTE *fmEncoded, unsigned int count);
    static int decodeMFM(BYTE *decoded, BYTE *fmEncoded, int count);
};

#endif

