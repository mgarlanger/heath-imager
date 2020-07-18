# heath-imager

Floppy Disk Imaging for Heathkit hard-sectored disks using Device Side's FC5025.

With permission from Device Side, this program used parts of Device Side's source code for interfacing
with the FC5025.

## Compiling

Currently heath-imager is running under Linux and Mac OS X. 

### Linux

Packages required
* libusb-devel
* gtk2

### Mac OS X
Use brew to install:
* libusb-compat
* gtk+


## Running

### Linux

You will need to either set permissions for the dev device to allow user access, or run the program as root, for the program to bbe able to query and find the FC5025 device.

### Mac OS X

No changes were needed, and the program did not have to run as root in order to find and use the FC5025 device.

