//! 
//! \file decode.cpp
//!
//! Process raw files from the FC5025, and decode either FM or MFM encoded buffers.
//!
//!

#include "decode.h"

#include <stdio.h>


//!  decodeFM()
//!
//!  @param decode        pointer to processed buffer
//!  @param fmEncoded     pointer to raw buffer
//!  @param count         number of bytes in the final processed file
//!
//!  @return number of errors (currently returns zero) 
//!
//!  \todo change count from bytes in decoded buffer to bytes in raw buffer.
//!
int
Decode::decodeFM(BYTE         *decoded,
                 BYTE         *fmEncoded,
                 unsigned int  count)
{
    State  state = none;

    unsigned int errorsZeros = 0;
    unsigned int errorsOnes  = 0;
    unsigned int lastBit     = 1;  // set to avoid warning of potentially uninitialized 
    unsigned int decodedValue;
    unsigned int encodedValue;

    
    while (count--)
    {
        // initialize to zero
        decodedValue = 0;

        encodedValue = (*fmEncoded++ << 8);
        encodedValue |= *fmEncoded++;

        // need to process 2 raw bytes to get a processed byte
        for (int shift = 14; shift >= 0; shift -= 2)
        {
        
            // save the 2 bits
            unsigned int  val = (encodedValue >> shift) & 0x3;
            switch (val)
            {   
                case 0:
                    // error, should never be 2 zeros in a row. remain in current state 
                    errorsZeros++;
                    lastBit = 0;
                    break;
                case 1:
                    // zero bit is in the high bit
                    // check current state
                    switch (state)
                    {
                        case none:
                            // new block, set state to hi
                            state = hi;
                            lastBit = 0;
                            break;
                        case lo:
                            // error, but type of error depends on last bit value
                            if (lastBit == 0) 
                            {
                                errorsZeros++;
                            }
                            else
                            {
                                errorsOnes++;
                            }
                            // change state
                            state = hi;
                            lastBit = 0;
                            break;
                        case hi:
                            // 0 bit detected
                            lastBit = 0;
                            break;
                    }
                    break;
                case 2:
                    // zero bit is in the low bit
                    switch (state)
                    {   
                        case none:
                            state = lo;
                            lastBit = 0;
                            break;
                        case lo:
                            // zero bit
                            lastBit = 0;
                            break;
                        case hi:
                            // lost or gained a bit, low bit should have been a clock, so set, but
                            // it's 0. transisition to the low state and note error.
                            state = lo;
                            errorsOnes++; 
                            lastBit = 0;
                            break;
                    }
                    break;
                case 3:
                    // double 1 bit - clock + 1 data, valid in all states.
                    // data bit is 1.
                    lastBit = 1;
                    break;
            }

            // shift to provide room for the new bit
            decodedValue <<= 1;
            decodedValue |=  lastBit;
        }

        // save the decoded value
        *decoded++ = decodedValue;
    }

    if (errorsZeros) 
    {
         printf("Unexpected Zeros: %d\n", errorsZeros);
    }
    if (errorsOnes) 
    {
         printf("Unexpected Ones: %d\n", errorsOnes);
    }

    return 0;
}


//!  decodeMFM()
//!
//!  @param decode        pointer to processed buffer
//!  @param fmEncoded     pointer to raw buffer
//!  @param count         number of bytes in the final processed file
//!
//!  @return number of errors (currently returns zero) 
//!
//!  \todo change count from bytes in decoded buffer to bytes in raw buffer.
//!  \todo implement
//!
int
Decode::decodeMFM(BYTE          *decoded,
                  BYTE          *mfmEncoded,
                  unsigned int   count)
{
#if 0
    BYTE *mfmData;
    int   bitNum;
    int   onePending;
    int   zeroPending;
    int   lastBit;
    BYTE  mfmByte;

    mfmData    = mfmEncoded;
    bitNum     = 0;
    onePending = 0;
    mfmByte    = *mfmData++;

    while(count--)
    {
        *decoded++ = getDecodedByte();
    }
#endif
 
    return 1;
}

