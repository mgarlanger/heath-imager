
#include "h17disk.h"
#include "hdos.h"

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>


const int directoryLength = 50;

static int usage(char *progName) {
	fprintf(stderr,"Usage: %s h17disk_file\n",progName);
	return 1;
}

int main(int argc, char *argv[]) {
    H17Disk *image = new(H17Disk);
    char directoryName[directoryLength];

    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }
    if (!image->loadFile(argv[1]))
    {
        printf("Unable to open file: %s\n", argv[1]);
        return 1;
    }
    
    char* ptr = strrchr(argv[1], '.');
    int length = ptr - argv[1];
    if (length > directoryLength) {
       length = directoryLength;
    }
    strncpy(directoryName, argv[1], length);
    mkdir(directoryName, 0777);
    chdir(directoryName);
 
    //printf("------------------------\n");
    //printf("  Read Complete\n");
    //printf("------------------------\n");
    //printf(" directoryName: %s\n", directoryName);

    //image->analyze();

    HDOS *hdos = new HDOS(image);

    hdos->dumpInfo();
    
    return 0; 
}

