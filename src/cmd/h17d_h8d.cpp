
#include "h17disk.h"

#include <stdio.h>


static int usage(char *progName) {
	fprintf(stderr,"Usage: %s h17disk_file h8d_file\n",progName);
	return 1;
}

int main(int argc, char *argv[]) {
    H17Disk *image = new(H17Disk);

    if (argc < 2 || argc > 3)
    {
        usage(argv[0]);
        return 1;
    }

    std::string infile(argv[1]);

    image->loadFile(infile.c_str());

    std::string outfile;

    if (argc == 2) 
    {
        outfile.assign(argv[1], infile.rfind("."));
        outfile.append(".h8d");
    }
    else
    {
        outfile.assign(argv[2]);
    }

    printf("------------------------\n");
    printf("  Read Complete\n");
    printf("------------------------\n");


    image->analyze();


    if (!image->saveAsH8D(outfile.c_str()))
    {
        printf("Unable to save file\n");
    };
    
    if (image)
    {
        delete image;
    } 
    return 0; 
}

