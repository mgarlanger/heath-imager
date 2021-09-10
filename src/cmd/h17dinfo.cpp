
#include "h17disk.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

char *progName;

static void usage()
{
    fprintf(stderr,"Usage: %s [-s] [-d n] h17disk_file\n",progName);
    exit(EXIT_FAILURE);
}

static int charToInt(char *num)
{
    if (strlen(num) != 1)
    {
        usage(); 
    }

    return num[0] - '0';
}

int main(int argc, char *argv[])
{
    progName = argv[0];

    H17Disk *image = new(H17Disk);

    char opt;
    bool summary = false;
    int  debugLevel = 9;

    while ((opt = getopt(argc, argv, "d:s")) != -1) {
        switch (opt) {
        case 's':
            summary = true;
            break;
        case 'd':
            debugLevel = charToInt(optarg);
            break;
        default: /* '?' */
            usage();
        }
    }
    if (optind >= argc) {
        usage();
    }

    printf("file: %s\n", argv[optind]);
    image->dumpFileInfo(argv[optind], debugLevel);

    exit(EXIT_SUCCESS);
}

