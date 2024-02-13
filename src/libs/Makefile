

ifeq ($(OS), Linux)
	ARFLAGS = crf
	CXXFLAGS   = -Wall -Werror -O0 -std=c++17 -g
else
	# Mac OS X
	ARFLAGS = rcs
	USB_HEADER_DIR = /opt/homebrew/Cellar/libusb-compat/0.1.8/include/
	CXXFLAGS   = -Wall -Werror -O0 -I$(USB_HEADER_DIR) -std=c++17 -g -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE
endif

CXX        = g++

OUTPUT_DIR = ../../output/libs/
_TARGET    = fc5025lib.a
TARGET     = $(OUTPUT_DIR)$(_TARGET)
_H17LIB    = h17disk.a
H17LIB     = $(OUTPUT_DIR)$(_H17LIB)
# make timestamp.. make sure everything get rebuilt with a makefile change.
#_MAKE_TS   = make.ts 
#MAKE_TS    = $(OUTPUT_DIR)$(_MAKE_TS)
SRCS       = decode.cpp disk.cpp drive.cpp heath_hs.cpp fc5025.cpp 
_OBJS      = $(SRCS:.cpp=.o)
OBJS       = $(addprefix $(OUTPUT_DIR),$(_OBJS))
DEPS       = $(OBJS:.o=.d)
H17SRCS    = h17disk.cpp h17block.cpp raw_track.cpp raw_sector.cpp sector.cpp track.cpp disk_util.cpp dump.cpp hdos.cpp cpm.cpp
_H17OBJS   = $(H17SRCS:.cpp=.o)
H17OBJS    = $(addprefix $(OUTPUT_DIR),$(_H17OBJS))
H17DEPS    = $(H17OBJS:.o=.d)

dummy_build_folder := $(shell mkdir -p $(OUTPUT_DIR))

.PHONY: clean all clean_light docs

all: $(MAKE_TS) $(TARGET) $(H17LIB)

$(TARGET): $(OBJS) 
	$(AR) $(ARFLAGS) $@ $^

$(H17LIB): $(H17OBJS) 
	$(AR) $(ARFLAGS) $@ $^

$(OUTPUT_DIR)%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

$(OUTPUT_DIR)%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) $< > $@

docs: $(SRCS) $(H17SRCS) 
	doxygen doxy.conf

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
