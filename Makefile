TARGET	:= metro

EXT_DEBUG			:= -debug
EXT_DEBUG_NOALERT	:= -debug-noalert

EXTENSIONS		:= \
	$(EXT_DEBUG) \
	$(EXT_DEBUG_NOALERT)

ALL_OUTPUT_FILES	:= \
	$(foreach e,$(EXTENSIONS),$(TARGET)$(e))

CC		= clang
CXX		= clang++
LD		= $(CXX)

BINDIR	= /usr/local/bin

TOPDIR	?= $(CURDIR)
BUILD	:= build
INCLUDE	:= include
SOURCES	:= src \
	src/GC \
	src/Parser \
	src/Types

OPTFLAGS		:= -O3
WARNFLAGS		:= -Wall -Wextra -Wno-switch
DBGFLAGS		:=
COMMONFLAGS	= $(DBGFLAGS) $(INCLUDES) $(OPTFLAGS) $(WARNFLAGS)
CFLAGS			:= $(COMMONFLAGS)
CXXFLAGS		:= $(CFLAGS) -std=c++20
LDFLAGS			:= -Wl,--gc-sections,-s

%.o: %.c
	@echo $(notdir $<)
	@$(CC) $(CFLAGS) -MP -MMD -MF $*.d -c -o $@ $<

%.o: %.cc
	@echo $(notdir $<)
	@$(CXX) $(CXXFLAGS) -MP -MMD -MF $*.d -c -o $@ $<

ifneq ($(notdir $(CURDIR)),$(BUILD))

export OUTPUT			= $(TOPDIR)/$(TARGET)

export VPATH		= $(foreach dir,$(SOURCES),$(TOPDIR)/$(dir))
export INCLUDES		= $(foreach dir,$(INCLUDE),-I$(TOPDIR)/$(dir))

CFILES			= $(notdir $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c)))
CXXFILES		= $(notdir $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cc)))

export OFILES		= $(CFILES:.c=.o) $(CXXFILES:.cc=.o)

.PHONY: $(BUILD) all debug release clean re install

DEBUGDIR	= $(BUILD)-debug

all: release debug debug_no_alert

release: $(BUILD)
	@echo release-build
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

debug: $(DEBUGDIR)
	@echo debug-build
	@$(MAKE) --no-print-directory \
		OUTPUT="$(OUTPUT)$(EXT_DEBUG)" BUILD=$(DEBUGDIR) OPTFLAGS="-O0 -g" \
		DBGFLAGS="-DMETRO_DEBUG -gdwarf-4" LDFLAGS="" \
		-C $(DEBUGDIR) -f $(CURDIR)/Makefile

debug_no_alert: $(DEBUGDIR_NO_ALERT)
	@echo debug-build \(no-alert\)
	@$(MAKE) --no-print-directory \
		OUTPUT="$(OUTPUT)$(EXT_DEBUG_NOALERT)" BUILD=$(DEBUGDIR) OPTFLAGS="-O0 -g" \
		DBGFLAGS="-DMETRO_DEBUG -DMETRO_NO_ALERT=1 -gdwarf-4" LDFLAGS="" \
		-C $(DEBUGDIR) -f $(CURDIR)/Makefile

$(BUILD):
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)

$(DEBUGDIR):
	@[ -d $(DEBUGDIR) ] || mkdir -p $(DEBUGDIR)

$(DEBUGDIR_NO_ALERT):
	@[ -d $(DEBUGDIR_NO_ALERT) ] || mkdir -p $(DEBUGDIR_NO_ALERT)

allbuilddir: $(BUILD) $(DEBUGDIR) $(DEBUGDIR_NO_ALERT)

clean:
	rm -rf $(BUILD) $(DEBUGDIR) $(TARGET)

clean-debug:
	rm -rf $(DEBUGDIR) $(TARGET)

re: clean all

run: cclear all
	@clear
	@echo run $(TARGET)
	@echo ----------------------------------------------------------------
	@./metro

install: all
	@echo install...
	@install $(notdir $(OUTPUT)) $(BINDIR)/$(TARGET)

cclear:
	@clear

else

DEPENDS	= $(OFILES:.o=.d)

$(OUTPUT): $(OFILES)
	@echo linking...
	@$(LD) $(LDFLAGS) -o $@ $^

-include $(DEPENDS)

endif