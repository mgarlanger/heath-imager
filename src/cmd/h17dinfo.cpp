
#include "h17disk.h"

#include <stdio.h>


static int usage(char *progName) {
	fprintf(stderr,"Usage: %s h17disk_file\n",progName);
	return 1;
}

int main(int argc, char *argv[]) {
    H17Disk *image = new(H17Disk);

    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }
    printf("h17dinfo: %s\n", argv[1]);

    image->decodeFile(argv[1]);
}

