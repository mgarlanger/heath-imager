// 
// Convert basic H8D images into IMD images. 
// 
// Currently, only handles single-sided/48 tpi disk images
//
//
// \todo - objectify program/remove globals.
//
// Mark Garlanger - http://heathkit.garlanger.com
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char BYTE;

BYTE image[102400];
BYTE checkSum_m;
BYTE volume_m;

bool cpmMode = false;


void printUsage(char *name)
{
    printf("%s (v1.02)\n", name);
    printf("Usage:\n");
    printf(" convert h8d file to imd format:\n");
    printf("    %s [-c] [-h] <h8d_file> <imd_file>\n\n", name);
    printf(" Options:\n");
    printf("    -c   convert a CP/M H8D image to IMD\n");
    printf("    -h   convert an HDOS H8D image to IMD (default)\n");
    exit(1);
}

bool loadH8D(const char *name)
{
    FILE *file;

    bool pass = true;

    if ((file = fopen(name, "r")) != NULL)
    {
        int num;
        if ((num = fread(image, 256, 10*40, file )) != 400)
        {
            printf("%s: read failed %s - %d\n", __FUNCTION__, name, num);
            pass = false;
        }

        fclose(file);
    }
    else
    {
        printf("%s: unable to open file - %s\n", __FUNCTION__, name);
        pass = false; 
    }

    if (pass) 
    {
        printf("Loaded: (%s): Success\n", name);
    }

    return(pass);
}

void writeHeaderInfo(FILE *file)
{
    fprintf(file, "IMD 1.02 (https://heathkit.garlanger.com/)\n");
    fprintf(file, "Automatic conversion from H8D hard-sectored to IMD soft-sectored image\n");
    fputc(0x1a, file);
}

void writeSector(FILE *file, int track, int sect)
{
    // for now, just normal data, no compression
    // sector type normal 
    fputc(0x01, file);

    // determine start of sector
    int base = (track * 2560) + ((sect - 1) * 256);

    // write 256 sector
    for (int pos = 0; pos < 256; pos++)
    {
        fputc(image[base + pos], file);
    }
}

void writeTrackHeader(FILE *file, int track, int head)
{
    // FM 300 kbps
    fputc(0x01, file);
    fputc(track & 0xff, file);
    fputc(head & 0xff, file);
    // sectors per track
    fputc(0x0a, file);
    // sector size (256)
    fputc(0x01, file);
}

void writeTrackHDOS(FILE *file, int track, int head)
{
    writeTrackHeader(file, track, head);

    // sector numbering map
    fputc(0x01, file);
    fputc(0x02, file);
    fputc(0x03, file);
    fputc(0x04, file);
    fputc(0x05, file);
    fputc(0x06, file);
    fputc(0x07, file);
    fputc(0x08, file);
    fputc(0x09, file);
    fputc(0x0a, file);
	
    for (int sect = 0; sect < 10; sect++)
    {

        writeSector(file, track, sect + 1);
    }


}

void writeTrackCPM(FILE *file, int track, int head)
{

    writeTrackHeader(file, track, head);

    /*// FM 300 kbps
    fputc(0x01, file);
    fputc(track & 0xff, file);
    fputc(head & 0x01, file);
    // sectors per track (10)
    fputc(0x0a, file);
    // sector size (256)
    fputc(0x01, file);*/

    // sector numbering map
    fputc(0x01, file);
    fputc(0x08, file);
    fputc(0x05, file);
    fputc(0x02, file);
    fputc(0x09, file);
    fputc(0x06, file);
    fputc(0x03, file);
    fputc(0x0a, file);
    fputc(0x07, file);
    fputc(0x04, file);

    // 
    int logicalMap[10] = { 1, 5, 9, 3, 7, 2, 6, 10, 4, 8 };
    // sector order
    int interleave[10] = { 1, 8, 5, 2, 9, 6, 3, 10, 7, 4 };

    for (int sect = 0; sect < 10; sect++)
    {
        writeSector(file, track, logicalMap[interleave[sect] - 1]);
    }
}


void saveIMD(const char *name)
{
    FILE *file;

    if ((file = fopen(name, "w")) != NULL)
    {
        writeHeaderInfo(file);

        for (int track = 0; track < 40; track++)
        {
            if (cpmMode)
            {
                writeTrackCPM(file, track, 0);
            }
            else
            {
                writeTrackHDOS(file, track, 0);
            }
        }
        fclose(file);
        printf("Save h17raw(%s): Success\n", name);
    }
    else
    {
        printf("%s: unable to open file - %s\n", __FUNCTION__, name);
    }
}

int main(int argc, char *argv[])
{
    char *in_name, *out_name;

    if (argc == 4)
    {
        if (strncmp(argv[1], "-c", 3) == 0)
        {
            cpmMode = true;
        }
        in_name = argv[2];
        out_name = argv[3];
    }
    else if (argc == 3)
    {
        in_name = argv[1];
        out_name = argv[2]; 
    }
    else
    {
        printUsage(argv[0]);
    }

    printf("H8D to IMD Conversion\n");
    printf("Converting %s image\n", cpmMode ? "a CP/M" : "an HDOS");

    if (loadH8D(in_name))
    {
        //processDisk();
        saveIMD(out_name);
    }

    return(0);
}

