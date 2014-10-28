#ifndef __SECTOR_H__
#define __SECTOR_H__

#include <iostream>
#include <fstream>
#include <cstdint>


class Sector
{
public:
    Sector(unsigned char  side,
           unsigned char  track,
           unsigned char  sector,
           unsigned char  error,
           unsigned char *buf,
           unsigned int   bufSize);
    ~Sector();

    bool writeToFile(std::ofstream &file);

private:
    unsigned int   bufSize_m;
    unsigned char *buf_m;
    unsigned char  side_m;
    unsigned char  track_m;
    unsigned char  sector_m;
    unsigned char  error_m;

};


#endif
