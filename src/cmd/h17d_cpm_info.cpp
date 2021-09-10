
#include "h17disk.h"
#include "cpm.h"

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <map>


const int maxDirectoryLength_c = 50;

static int usage(char *progName) {
	fprintf(stderr,"Usage: %s h17disk_file\n",progName);
	return 1;
}

int main(int argc, char *argv[]) {
    H17Disk *image = new(H17Disk);
    char directoryName[maxDirectoryLength_c + 1];

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
    if (length > maxDirectoryLength_c) {
       length = maxDirectoryLength_c;
    }
    strncpy(directoryName, argv[1], length);
    mkdir(directoryName, 0777);
    chdir(directoryName);
 
    //printf("------------------------\n");
    //printf("  Read Complete\n");
    //printf("------------------------\n");
    //printf(" directoryName: %s\n", directoryName);

    //image->analyze();

    CPM *cpm = new CPM(image);

    cpm->dumpInfo();

    cpm->saveAllFiles();
    
    return 0; 
}

