
#include "h17disk.h"

#include <unistd.h>
#include <stdio.h>


static int usage(char *progName) {
	fprintf(stderr,"Usage: %s [-s] h17disk_file\n",progName);
	fprintf(stderr,"Usage: %s [-s] h17disk_file\n",progName);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    H17Disk *image = new(H17Disk);

    bool summary = false;
    int opt;

    while ((opt = getopt(argc, argv, "s")) != -1) {
        switch (opt) {
        case 's':
            summary = true;
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }
    if (optind >= argc) {
        usage(argv[0]);
    }

    printf("h17dinfo: %s\n", argv[optind]);
    if (summary) {
        printf("just summarize\n");
    }

    image->decodeFile(argv[optind], summary);

    exit(EXIT_SUCCESS);
}

