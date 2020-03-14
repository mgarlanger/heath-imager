
#include "h17disk.h"

#include <stdio.h>


static int usage(char *progName) {
	fprintf(stderr,"Usage: %s h17disk_file\n",progName);
	return 1;
}

int main(int argc, char *argv[]) {
    H17Disk image;

    if (argc != 3)
    {
        usage(argv[0]);
        return 1;
    }
    image.decodeFile(argv[1]);

    // TODO reprocess raw blocks to get a possibly better image.
    // For all sectors with an error, 
    //    1) pick processed raw block with highest error number (lowest type of error). 
    //    2) process header and data portions separately
    //
    image.reprocessFile();

    image.saveFile(argv[2]);
}

