
#include "h17disk.h"

#include <stdio.h>


static int usage(char *progName) {
	fprintf(stderr,"Usage: %s h17disk_file h8d_file\n",progName);
	return 1;
}

int main(int argc, char *argv[]) {
    H17Disk *image = new(H17Disk);

    if (argc != 3)
    {
        usage(argv[0]);
        return 1;
    }
    image->openForRecovery(argv[1]);

    printf("------------------------\n");
    printf("  Read Complete\n");
    printf("------------------------\n");


    image->analyze();


    image->saveAsH8D(argv[2]);
    
    if (image)
    {
        delete image;
    } 
    return 0; 
}

