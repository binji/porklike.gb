# If you move this project you can change the directory
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
GBDK_HOME = ../gbdk/
LCC = $(GBDK_HOME)bin/lcc
RGBGFX = ../rgbds/rgbgfx
GBCOMPRESS = $(GBDK_HOME)bin/gbcompress
PYTHON = python

# Set platforms to build here, spaced separated. (These are in the separate Makefile.targets)
# They can also be built/cleaned individually: "make gg" and "make gg-clean"
# Possible are: gb gbc pocket megaduck sms gg
TARGETS=gb pocket # megaduck sms gg

# Configure platform specific LCC flags here:
LCCFLAGS_gb      = -Wm-ys #
LCCFLAGS_pocket  = -Wm-ys # Usually the same as required for .gb
LCCFLAGS_duck    = -Wm-ys # Usually the same as required for .gb
LCCFLAGS_gbc     = -Wm-ys -Wm-yc # Same as .gb with: -Wm-yc (gb & gbc) or Wm-yC (gbc exclusive)
LCCFLAGS_sms     =
LCCFLAGS_gg      =

LCCFLAGS += $(LCCFLAGS_$(EXT)) # This adds the current platform specific LCC Flags

LCCFLAGS += -Wl-j -Wl-m -Wl-w -Wm-yo4 -Wm-yt1 -Wm-ynPORKLIKE
LCCFLAGS += -Wl-b_CALIGNED=0x0200 -Wl-b_CODE=0x300 -Wl-b_DALIGNED=0xc100 -Wl-b_DATA=0xcf00
# LCCFLAGS += -debug # Uncomment to enable debug output
# LCCFLAGS += -v     # Uncomment for lcc verbose output

# You can set the name of the ROM file here
PROJECTNAME = porklike

# EXT?=gb # Only sets extension to default (game boy .gb) if not populated
SRCDIR      = src
IMGDIR      = img
OBJDIR      = obj/$(EXT)
BINDIR      = build/$(EXT)
MKDIRS      = $(OBJDIR) $(BINDIR) # See bottom of Makefile for directory auto-creation

CSOURCES = bank0.c gen.c main.c sound.c title.c rand.c mob.c pickup.c ai.c gameplay.c inventory.c palette.c spr.c float.c msg.c sprite.c targeting.c gameover.c
ASMSOURCES = util.s music.s aligned.s
IMGSOURCES = bg.png shared.png sprites.png dead.png title.png

BINS	    = $(OBJDIR)/$(PROJECTNAME).$(EXT)
OBJS       = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o) $(IMGSOURCES:%.png=$(OBJDIR)/tile%.o)

# Builds all targets sequentially
all: $(TARGETS)

# Generated file dependencies
$(SRCDIR)/main.c: $(OBJDIR)/tilebg.o $(OBJDIR)/tileshared.o $(OBJDIR)/tilesprites.o $(OBJDIR)/tiledead.o
$(SRCDIR)/title.c: $(OBJDIR)/tiletitle.o

# Compile .c files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	$(LCC) $(CFLAGS) -Wf-MD -I$(OBJDIR) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.s
	$(LCC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/tile%.o: $(IMGDIR)/%.png
	$(RGBGFX) -u $< -o $(basename $@).bin
	$(GBCOMPRESS) $(basename $@).bin $(basename $@).binz
	$(PYTHON) ref/bin2c.py $(basename $@).binz -o $(basename $@).c
	$(LCC) $(CFLAGS) -c -o $@ $(basename $@).c

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(LCC) $(LCCFLAGS) $(CFLAGS) -o $(BINDIR)/$(PROJECTNAME).$(EXT) $(OBJS)

clean:
	@echo Cleaning
	@for target in $(TARGETS); do \
		$(MAKE) $$target-clean; \
	done

# Include available build targets
include Makefile.targets


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
ifneq ($(strip $(EXT)),)           # Only make the directories if EXT has been set by a target
$(info $(shell mkdir -p $(MKDIRS)))
endif

-include $(OBJDIR)/%.d
