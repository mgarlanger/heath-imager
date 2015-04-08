// 
// decode.cpp
//
// Process raw files from the FC5025, and decode either FM or MFM encoded buffers.
//
//

#include "decode.h"

#include <stdio.h>


//  decodeFM()
//
//  @param decode        pointer to processed buffer
//  @param fmEncoded     pointer to raw buffer
//  @param count         number of bytes in the final processed file
//
//  @return number of errors (currently returns zero) 
//
//  \TODO change count from bytes in decoded buffer to bytes in raw buffer.
//
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

        //printf("encodedValue: %04x\n", encodedValue);
        // need to process 2 raw bytes to get a processed byte
        for (int shift = 14; shift >= 0; shift -= 2)
        {
        
            // save the 2 bits
            unsigned int  val = (encodedValue >> shift) & 0x3;
            //printf("shift: %d  val: %d\n", shift, val); 
            switch (val)
            {   
                case 0:
                    // error, should never be 2 zeros in a row. remain in current state 
                    errorsZeros++;
                    //printf("Case 0\n");
                    lastBit = 0;
                    break;
                case 1:
                    // zero bit is in the high bit
                    switch (state)
                    {
                        case none:
                            state = hi;
                            lastBit = 0;
                            break;
                        case lo:
                            // error, but type of error depends on last bit value
                            if (lastBit == 0) 
                            {
                                errorsZeros++;
                                //printf("Case lo, zero\n");
                            }
                            else
                            {
                                errorsOnes++;
                                //printf("Case lo, one\n");
                            }
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
                            //printf("Case hi, one\n");
                            lastBit = 0;
                            break;
                    }
                    break;
                case 3:
                    // double 1 bits
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
         printf("errorsZero: %d\n", errorsZeros);
    }
    if (errorsOnes) 
    {
         printf("errorsOne: %d\n", errorsOnes);
    }

    return 0;
}

//  decodeFM()
//
//  @param decode        pointer to processed buffer
//  @param fmEncoded     pointer to raw buffer
//  @param count         number of bytes in the final processed file
//
//  @return number of errors (currently returns zero) 
//
//  \TODO change count from bytes in decoded buffer to bytes in raw buffer.
//
int
Decode::decodeFM_old2(BYTE         *decoded,
                 BYTE         *fmEncoded,
                 unsigned int  count)
{
    State  state = none;

    unsigned int errorsZeros = 0;
    unsigned int errorsOnes  = 0;
    unsigned int lastBit     = 1;  // set to avoid warning of potentially uninitialized 

    
    while (count--)
    {
        // initialize to zero
        unsigned int decodedValue = 0;

        // could - do 2 bytes at a time.
        // need to process 2 raw bytes to get a processed byte
        for (int i = 0; i < 2; i++)
        {
            // start at the top 2 bits
            unsigned int shift = 6;

            // check each pair of bits
            for (int j = 0; j < 4; j++)
            {
        
                // save the 2 bits
                unsigned int  val = (*fmEncoded >> shift) & 0x3;
                shift -= 2;

                // processing depends on current state 
                switch (state)
                {
                    // nothing yet, use the 2 bits to try to determin it.
                    case none:
                        switch (val)
                        {
                            case 0:
                                // error, shouldn't be 2 zeros in a row. remain in 'none'
                                errorsZeros++;
                                lastBit = 0;
                                break;
                            case 1:
                                // zero bit is in the high bit
                                state = hi;
                                lastBit = 0;
                                break;
                            case 2:
                                // zero bit is in the low bit
                                state = lo;
                                lastBit = 0;
                                break;
                           case 3:
                                // 2 set bits, still don't know the state to transistion into
                                // but the bit value has to be 1.
                                lastBit = 1;
                                break;
                        }
                        break;

                    case hi:
                        switch (val)
                        {
                            case 0:
                                // error, shouldn't be 2 zeros in a row.
                                errorsZeros++;
                                lastBit = 0;
                                break;
                            case 1:
                                // 0 bit detected
                                lastBit = 0;
                                break;
                            case 2:
                                // lost or gained a bit, low bit should have been a clock, so set, but
                                // it's 0. transisition to the low state and note error.
                                state = lo;
                                errorsOnes++; 
                                lastBit = 0;
                                break;
                            case 3:
                                // data bit is 1. 
                                lastBit = 1;
                                break;
                        } 
                        break;

                    case lo:
                        switch (val)
                        {
                            case 0:
                                // error, shouldn't be 2 zeros in a row.
                                errorsZeros++;
                                lastBit = 0;
                                break;
                            case 1:
                                // error, but type of error depends on last bit value
                                if (lastBit == 0) 
                                {
                                    errorsZeros++;
                                }
                                else
                                {
                                    errorsOnes++;
                                }
                                state = hi;
                                lastBit = 0;
                                break;
                            case 2:
                                // zero bit
                                lastBit = 0;
                                break;
                            case 3:
                                // one bit
                                lastBit = 1;
                                break;
                        } 
                        break;

                }
                decodedValue <<= 1;
                decodedValue |=  lastBit;
                    
            }

            fmEncoded++;
        }

        *decoded++ = decodedValue;
    }

    if (errorsZeros) 
    {
         printf("errorsZero: %d\n", errorsZeros);
    }
    if (errorsOnes) 
    {
         printf("errorsOne: %d\n", errorsOnes);
    }

    return 0;
}


//
// count is the number of output (decoded) bytes.
// \TODO count should be the encoded size, and return the size of the decoded buffer.
int  
Decode::decodeFM_old(BYTE         *decoded,
                     BYTE         *fmEncoded, 
                     unsigned int  count)
{
    int  status = 0;
    bool dA     = true;
    int  errors = 0;

    // if there are still bytes to decode
    while(count--)
    {
        // if currently in clock pattern 'A'  (1010)
        if (dA)
        {
            // see if it still is valid
            if ((*fmEncoded & 0xAA) != 0xAA)
            {
                // if not see if clock '5' is valid (0101)
                if ((*fmEncoded & 0x55) == 0x55)
                {
                    // switch to the '0101' pattern
                    dA = false;
                }
                else
                {
                    // neither could be the total clock, count as an error.
                    errors++;
                }
            }
        }
        else
        {
            // see if '5' is no longer a valid clock pattern
            if ((*fmEncoded & 0x55) != 0x55)
            {
                // if not check for 'A'
                if ((*fmEncoded & 0xAA) == 0xAA)
                {
                    // switch to 'A'
                    dA = true;
                }
                else
                {
                    // flag as an error.
                    errors++;
                }
            }
        }
        if (dA)
        {
            BYTE v = *fmEncoded++;
            *decoded = ((v & 0x40) << 1) | ((v & 0x10) << 2) | ((v & 0x04) << 3) | ((v & 0x01) << 4);
            v = *fmEncoded++;
            *decoded++ |= ((v & 0x40) >> 3) | ((v & 0x10) >> 2) | ((v & 0x04) >> 1) | (v & 0x01);
        }
        else
        {
            BYTE v = *fmEncoded++;
            *decoded = (v & 0x80) | ((v & 0x20) << 1) | ((v & 0x08) << 2) | ((v & 0x02) << 3);
            v = *fmEncoded++;
            *decoded++ |= ((v & 0x80) >> 4) | ((v & 0x20) >> 3) | ((v & 0x08) >> 2) | ((v & 0x02) >> 1);
        }
    }
   
    if (errors)
    {
        // currently don't count errors... let the decoding see if it can decode a valid sector
        //status = 1;
        //printf("Total errors = %d\n", errors);
    } 
    return status;
}
   

int
Decode::decodeMFM(BYTE *decoded,
                  BYTE *fmEncoded,
                  int   count)
{


    return 1;
}


