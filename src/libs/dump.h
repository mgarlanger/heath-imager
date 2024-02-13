//! \file dump.h
//!
//! Routines to dump data from objects and structures.
//!

#ifndef __DUMP_H__
#define __DUMP_H__


#include <cstdint>


void dumpDataBlock(uint8_t  buf[],
                   uint32_t length);

#endif // __DUMP_H__
