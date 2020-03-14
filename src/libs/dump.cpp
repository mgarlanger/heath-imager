//! \file dump.cpp
//!
//! Routines to display data dumps.
//!

#include "dump.h"

#include <cstdio>
#include <cctype>


//! dumpDataBlock()
//!
//! @param buf     buffer data
//! @param length  length of buffer
//!
//! @return void
//!
void dumpDataBlock(unsigned char buf[], unsigned int length) {

    uint8_t printAble[16];
    unsigned int i = 0;
    unsigned int pos = 4;

    for (i = 0; i < length; i++)
    {
        printAble[i % 16] = isprint(buf[pos]) ? buf[pos] : '.';
        if  ((i % 16) == 0)
        {
            printf("        %03d: ", i);
        }
        printf("%02x", buf[pos]);

        if ((i % 16) == 7)
        {
            printf(" ");
        }
        if ((i % 16) == 15)
        {
            printf("        |");
            for(int i = 0; i < 8; i++)
            {
                printf("%c", printAble[i]);
            }
            printf("  ");
            for(int i = 8; i < 16; i++)
            {
                printf("%c", printAble[i]);
            }

            printf("|\n");
        }
        pos++;
    }

    // check to see if we need to show the printable characters (in the right position
    if ((i % 16) != 0)
    {
        unsigned int pos = i % 16;
        unsigned int numSpaces = (16 - pos ) * 2 + ((16 - pos) >> 3);
        while(numSpaces--) {
            printf(" ");
        }
        printf("        |");
        for(unsigned int i = 0; i < 8; i++)
        {
            if (i < pos)
            {
                printf("%c", printAble[i]);
            }
            else
            {
                printf(" ");
            }
        }
        printf("  ");
        for(unsigned int i = 8; i < 16; i++)
        {
            if (i < pos)
            {
                printf("%c", printAble[i]);
            }
            else
            {
                printf(" ");
            }
        }

        printf("|\n");
    }

    printf("\n");
}

