TARGET      := AlterEgo
TITLE       := BHU100003
GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)

# Libraries
# Added -lSceNet_stub for networking and -lScePower_stub for battery
LIBS = -limgui -lvitaGL -lc -lSceCommonDialog_stub -lSceLibKernel_stub \
    -lSceDisplay_stub -lSceGxm_stub -lSceSysmodule_stub -lSceCtrl_stub \
    -lSceTouch_stub -lm -lSceAppMgr_stub -lSceAppUtil_stub -lmathneon \
    -lSceKernelDmacMgr_stub -lvitashark -lSceShaccCgExt -ltaihen_stub \
    -lSceShaccCg_stub -lSceAudioIn_stub -lSceMotion_stub -lScePower_stub \
    -lSceNet_stub

CPPSOURCES  := .

CFILES      := 
CPPFILES    := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES    := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS        := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX     = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O2 -g -mtune=cortex-a9 -mfpu=neon
CXXFLAGS = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

# VPK Packaging Rule
# Maps local assets in sce_sys/ to the VPK internal structure
$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) -d ATTRIBUTE2=12 "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
		--add sce_sys/icon0.png=sce_sys/icon0.png \
		--add sce_sys/pic0.png=sce_sys/pic0.png \
		--add sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
		--add sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
		--add sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
		$@

%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS) $(TARGET).elf.unstripped.elf $(TARGET).vpk eboot.bin param.sfo
