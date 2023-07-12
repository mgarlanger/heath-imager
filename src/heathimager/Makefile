
GTK_VERSION=gtk2

CPP=g++
CFLAGS=-std=c++11 -g -O0 -fno-inline
# CFLAGS=-std=c++0x -g -O0 -fno-inline
OUTPUT_DIR=../../output/executable
OUTPUT_PROG=../../output
PROG=heathimager

OBJDIR=$(OUTPUT_DIR)
DUMMY:=$(shell mkdir -p $(OBJDIR))

GTK2FLAGS=`pkg-config gtk+-2.0 --cflags`
GTK2LIBS=`pkg-config gtk+-2.0 --libs`
GTK3FLAGS=`pkg-config gtk+-3.0 --cflags`
GTK3LIBS=`pkg-config gtk+-3.0 --libs`
INCLUDES=-I../libs
FC5025_A=$(OUTPUT_DIR)/../libs/fc5025lib.a
H17DISK_A=$(OUTPUT_DIR)/../libs/h17disk.a

ifeq "$(GTK_VERSION)" "gtk3"
  GTKFLAGS=$(GTK3FLAGS)
  GTKLIBS=$(GTK3LIBS)
else
  GTKFLAGS=$(GTK2FLAGS)
  GTKLIBS=$(GTK2LIBS)
endif

HOST_OS=$(shell uname -s)

all: linux

linux: $(OUTPUT_DIR)/$(PROG)
	cp $(OUTPUT_DIR)/$(PROG) $(OUTPUT_PROG)

$(OUTPUT_DIR)/$(PROG).o: $(PROG).cpp
	$(CPP) -o $@ $(CFLAGS) $(GTKFLAGS) $(INCLUDES) -c $<

$(OUTPUT_DIR)/$(PROG): $(OUTPUT_DIR)/$(PROG).o $(FC5025_A) $(H17DISK_A)
	$(CPP) -o $@ $^ $(BACKEND_A) -lusb $(GTKLIBS)

clean:
	rm -rf $(OUTPUT_DIR)

