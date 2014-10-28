#include "decode.h"

#include <stdio.h>


// count is the number of output (decoded) bytes.
int  
Decode::decodeFM(BYTE *decoded,
                 BYTE *fmEncoded, 
                 int   count)
{
    int  status = 0;
    bool dA     = true;
    int  errors = 0;

    while(count--)
    {
        if (dA)
        {
            if ((*fmEncoded & 0xAA) != 0xAA)
            {
                if ((*fmEncoded & 0x55) == 0x55)
                {
                    dA = false;
                }
                else
                {
                    errors++;
                }
            }
        }
        else
        {
            if ((*fmEncoded & 0x55) != 0x55)
            {
                if ((*fmEncoded & 0xAA) == 0xAA)
                {
                    dA = true;
                }
                else
                {
                    errors++;
                }
            }
        }
        if (dA)
        {
            //BYTE v = *fmEncoded++;
            // *decoded = ((v & 0x40) << 1) | ((v & 0x10) << 2) | ((v & 0x04) << 3) | ((v & 0x01) << 4)
            // v = *fmEncoded++;
            // *decoded |= ((v & 0x40) >> 3) | ((v & 0x10) >> 2) | ((v & 0x04) >> 1) | (v & 0x01))
            // ----
            // BYTE v = *fmEncoded & 0x55;
            *decoded = 0;
            if((*fmEncoded) & 0x40)
            {
                *decoded |= 0x80;
            }
            if((*fmEncoded) & 0x10)
            {
                *decoded |= 0x40;
            }
            if((*fmEncoded) & 0x04)
            {
                *decoded |= 0x20;
            }
            if((*fmEncoded) & 0x01)
            {
                *decoded |= 0x10;
            }
            fmEncoded++;

            if((*fmEncoded) & 0x40)
            {
                *decoded |= 0x08;
            }
            if((*fmEncoded) & 0x10)
            {
                *decoded |= 0x04;
            }
            if((*fmEncoded) & 0x04)
            {
                *decoded |= 0x02;
            }
            if((*fmEncoded) & 0x01)
            {
                *decoded |= 0x01;
            }
            fmEncoded++;
            decoded++;
        }
        else
        {
            //BYTE v = *fmEncoded;
            // *decoded = (v & 0x80) | ((v & 0x20) << 1) | ((v & 0x08) << 2) | ((v & 0x02) << 3)
            // *decoded |= ((v & 0x80) >> 4) | ((v & 0x20) >> 3) | ((v & 0x08) >> 2) | ((v & 0x02) >> 1)
            *decoded = 0;
            if((*fmEncoded) & 0x80)
            {
                *decoded |= 0x80;
            }
            if((*fmEncoded) & 0x20)
            {
                *decoded |= 0x40;
            }
            if((*fmEncoded) & 0x08)
            {
                *decoded |= 0x20;
            }
            if((*fmEncoded) & 0x02)
            {
                *decoded |= 0x10;
            }
            fmEncoded++;

            if((*fmEncoded) & 0x80)
            {
                *decoded |= 0x08;
            }
            if((*fmEncoded) & 0x20)
            {
                *decoded |= 0x04;
            }
            if((*fmEncoded) & 0x08)
            {
                *decoded |= 0x02;
            }
            if((*fmEncoded) & 0x02)
            {
                *decoded |= 0x01;
            }
            fmEncoded++;
            decoded++;
        }
    }
   
    if (errors)
    {
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

