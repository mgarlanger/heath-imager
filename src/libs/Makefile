

CXX        = g++
CXXFLAGS   = -Wall -O3 -std=c++11
LDFLAGS    =
OUTPUT_DIR = ../../output/libs/
_TARGET    = fc5025lib.a
TARGET     = $(OUTPUT_DIR)$(_TARGET)
_H17LIB    = h17disk.a
H17LIB     = $(OUTPUT_DIR)$(_H17LIB)
# make timestamp.. make sure everything get rebuilt with a makefile change.
#_MAKE_TS   = make.ts 
#MAKE_TS    = $(OUTPUT_DIR)$(_MAKE_TS)
SRCS       = drive.cpp decode.cpp disk.cpp heath_hs.cpp fc5025.cpp 
_OBJS      = $(SRCS:.cpp=.o)
OBJS       = $(addprefix $(OUTPUT_DIR),$(_OBJS))
DEPS       = $(OBJS:.o=.d)
H17SRCS    = h17disk.cpp h17block.cpp raw_track.cpp raw_sector.cpp sector.cpp
_H17OBJS   = $(H17SRCS:.cpp=.o)
H17OBJS    = $(addprefix $(OUTPUT_DIR),$(_H17OBJS))
H17DEPS    = $(H17OBJS:.o=.d)

dummy_build_folder := $(shell mkdir -p $(OUTPUT_DIR))

.PHONY: clean all clean_light

all: $(MAKE_TS) $(TARGET) $(H17LIB)

$(TARGET): $(OBJS) 
	$(AR) crf $@ $^

$(H17LIB): $(H17OBJS) 
	$(AR) crf $@ $^

$(OUTPUT_DIR)%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

$(OUTPUT_DIR)%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) $< > $@

#$(MAKE_TS): Makefile
#	rm -f $(OBJS) $(DEPS) $(TARGET) $(H17OBJS) $(H17DEPS) $(H17LIB)
#	touch $@
#	$(MAKE)

clean: 
	rm -f $(OBJS) $(DEPS) $(TARGET) $(H17OBJS) $(MAKE_TS) \
	$(H17DEPS) $(H17LIB)

clean_light:
	rm -f $(OBJS) $(DEPS) $(TARGET) $(H17OBJS) \
	$(H17DEPS) $(H17LIB)


-include $(DEPS)
-include $(H17DEPS)

