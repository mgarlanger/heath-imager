CXX     = g++

OUTPUT_DIR=../../output/cmd/
OUTPUT_PROG=../../output/

HOST_OBJS=h17dinfo h17d_reprocess
HOST_PROGS=$(addprefix $(OUTPUT_DIR), $(HOST_OBJS)) $(OUTPUT_PROG)
CXXFLAGS=-I../libs -Wall -O3 -std=c++11
FC5025_A=$(OUTPUT_PROG)/libs/fc5025lib.a
H17DISK_A=$(OUTPUT_PROG)/libs/h17disk.a

dummy_build_folder := $(shell mkdir -p $(OUTPUT_DIR))

.PRECIOUS: $(OUTPUT_DIR)%.o

all: linux

linux: $(HOST_PROGS)
	cp $(addprefix $(OUTPUT_DIR), $(HOST_OBJS)) $(OUTPUT_PROG)/.

#$(HOST_PROGS): $(OUTPUT_DIR)$(HOST_OBJS).o $(H17DISK_A)
#	$(CXX) $(LDFLAGS) -o $@ $^ $(H17DISK_A)
	

$(OUTPUT_DIR)%: $(OUTPUT_DIR)%.o $(H17DISK_A) $(FC5025_A)
	$(CXX) $(LDFLAGS) -o $@ $^


$(OUTPUT_DIR)%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(OUTPUT_DIR) $(addprefix $(OUTPUT_PROG), $(HOST_OBJS))

