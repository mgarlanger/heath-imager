
#include "h17disk.h"
#include "cpm.h"
#include "hdos.h"
#include "h17block.h"

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

//#include <map>


const int maxDirectoryLength_c = 100;

static int usage(char *progName)
{
	fprintf(stderr,"Usage: %s h17disk_file\n",progName);
	return 1;
}


bool 
writeLabel(H17Disk *image)
{

    H17Block* block = image->getH17Block(H17Disk::LabelBlock_c);

    if (!block)
    {
        printf("No label block\n");
        return false;
    }

    uint32_t size = block->getDataSize();

    // disregard if only a null character
    if (size < 2)
    {
        printf("No data in label block\n");
        return false;
    }
 
    uint8_t *data = block->getData();

    FILE *labelFile = fopen("disk.label", "w");

    if (!labelFile)
    {
        printf("unable to open label file\n");
        return false;
    }
    
    fwrite(data, size-1, 1, labelFile);
    return true;
}


int 
main(int argc, char *argv[])
{

    char directoryName[maxDirectoryLength_c + 1];

    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    H17Disk *image = new(H17Disk);
    if (!image->loadFile(argv[1]))
    {
        printf("Unable to open file: %s\n", argv[1]);
        return 1;
    }
    
    char* ptr = strrchr(argv[1], '.');
    int length = ptr - argv[1];
    printf(" length: %d\n", length);
    if (length > maxDirectoryLength_c) {
       length = maxDirectoryLength_c;
    }
    strncpy(directoryName, argv[1], length);
    directoryName[length] = 0;
    mkdir(directoryName, 0777);
    chdir(directoryName);
 
    //printf("------------------------\n");
    //printf("  Read Complete\n");
    //printf("------------------------\n");
    printf(" directoryName: %s\n", directoryName);


    bool isValidCPM = CPM::isValidImage(*image);
    bool isValidHDOS = HDOS::isValidImage(*image);

    printf("isValidCPM: %d, isValidHDOS: %d\n", isValidCPM, isValidHDOS);

    writeLabel(image);
    if (isValidHDOS)
    {
        if (isValidCPM) 
        {
            // both are valid, create hdos directory, chdir 
            mkdir("hdos", 0777);
            chdir("hdos");
        }
        HDOS *hdos = new HDOS(image);
        // extract files 
        hdos->dumpInfo();
 
    }

    if (isValidCPM)
    {

        if (isValidHDOS)
        {
            // chmod back to original
            chdir("..");
            // create cpm directory, chdir
            mkdir("cpm", 0777);
            chdir("cpm");
        }

        CPM *cpm = new CPM(image);

        // extract CPM files
        cpm->saveAllFiles();
    }
 
    return 0; 
}

