# ARMCC makefile scripts and rules.

# Verbose
ifeq ($(USE_VERBOSE_COMPILE),yes)
  Q :=
  ECHO := @true
else
  Q := @
  ECHO := @echo
endif

# Automatic compiler options
OPT = $(USE_OPT)
COPT = $(USE_COPT)
CPPOPT = $(USE_CPPOPT)

# Use Microlib
COPT += -D__MICROLIB --li -g --apcs=interwork --split_sections
ASOPT += --li -g --apcs=interwork --pd "__MICROLIB SETA 1"

# Garbage collection
ifeq ($(USE_LINK_GC),yes)
else
endif

# Linker extra options
ifneq ($(USE_LDOPT),)
  LDOPT := $(LDOPT),$(USE_LDOPT)
endif

# Link time optimizations
ifeq ($(USE_LTO),yes)
  OPT += -flto
endif

# FPU-related options
ifeq ($(USE_FPU),)
  USE_FPU = no
endif

# Output directory and files
ifeq ($(BUILDDIR),)
  BUILDDIR = build
endif
ifeq ($(BUILDDIR),.)
  BUILDDIR = build
endif
ifeq ($(LDSCRIPT),)
OUTFILES = $(BUILDDIR)/$(PROJECT)_$(CPU)_$(TOOLCHAIN).lib
else
OUTFILES = $(BUILDDIR)/$(PROJECT).axf $(BUILDDIR)/$(PROJECT).hex \
           $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT).txt

ifdef SREC
OUTFILES += $(BUILDDIR)/$(PROJECT).srec
endif
endif
ifeq ($(CPU),host)
OUTFILES += $(BUILDDIR)/$(PROJECT).exe
endif

# Source files groups and paths
SRC	      = $(CSRC)$(CPPSRC)
SRCPATHS  = $(sort $(dir $(ASMXSRC)) $(dir $(ASMSRC)) $(dir $(SRC)))

# Various directories
OBJDIR    = $(BUILDDIR)/obj
LSTDIR    = $(BUILDDIR)/lst

# Object files groups
CXOBJS    = $(addprefix $(OBJDIR)/, $(notdir $(CXSRC:.c=.o)))
COBJS     = $(addprefix $(OBJDIR)/, $(notdir $(CSRC:.c=.o)))
CPPOBJS   = $(addprefix $(OBJDIR)/, $(notdir $(CPPSRC:.cpp=.o)))
ASMOBJS   = $(addprefix $(OBJDIR)/, $(notdir $(ASMSRC:.s=.o)))
ASMXOBJS  = $(addprefix $(OBJDIR)/, $(notdir $(ASMXSRC:.s=.o)))
OBJS	  = $(ASMXOBJS) $(ASMOBJS) $(CXOBJS) $(COBJS) $(CPPOBJS)

# Paths
IINCDIR = $(patsubst %,-I%,$(INCDIR) $(DINCDIR) $(UINCDIR))
LLIBDIR = $(patsubst %,-L%,$(DLIBDIR) $(ULIBDIR))

# Macros
DEFS    = $(DDEFS) $(UDEFS)
ADEFS   = $(DADEFS) $(UADEFS)

# Libs
LIBS    = $(DLIBS) $(ULIBS)

# Various settings
ifeq ($(CPU),host)
MCFLAGS   = #-m32
#else ifeq ($(CPU),hm1001)
#MCFLAGS   = --cpu $(CPU)
else
MCFLAGS   = --cpu $(CPU)
endif
ODFLAGS	  = -h -x --syms -S
ASFLAGS   = $(MCFLAGS) $(ASOPT)
ASXFLAGS  = $(MCFLAGS) $(ASOPT)
CFLAGS    = $(MCFLAGS) $(OPT) $(COPT)   $(CWARN) $(DEFS)
CPPFLAGS  = $(MCFLAGS) $(OPT) $(CPPOPT) $(CPPWARN) $(DEFS)
LDFLAGS   = $(MCFLAGS) --library_type=microlib --inline  --strict --summary_stderr --info summarysizes --map --callgraph --symbols --info sizes --info totals  --info veneers --info inline --diag_suppress=L6314 --entry=Reset_Handler --scatter $(LDSCRIPT) --list $(BUILDDIR)/$(PROJECT).map --symdefs=$(BUILDDIR)/$(PROJECT).symbols
#LDFLAGS   = $(MCFLAGS) --library_type=microlib --inline  --strict --info sizes --entry=Reset_Handler --scatter $(LDSCRIPT) --list $(BUILDDIR)/$(PROJECT).map --symdefs=$(BUILDDIR)/$(PROJECT).symbols

# Generate dependency information
#ASFLAGS  += --depend .dep/$(@F).d
#CFLAGS   += --depend .dep/$(basename $(@F)).d
#CPPFLAGS += --depend .dep/$(@F).d

# Paths where to search for sources
VPATH     = $(SRCPATHS)

#
# Makefile rules
#

all: PRE_MAKE_ALL_RULE_HOOK $(OBJS) $(OUTFILES) POST_MAKE_ALL_RULE_HOOK
	echo "all"

PRE_MAKE_ALL_RULE_HOOK:
ifeq ($(OS),Windows_NT)
	@echo -n "#define SVN_REV " > svn_rev.h
	@echo "\"$(shell svnversion -n .)\"" >> svn_rev.h
	@echo -n "#define SVN_REV_NUM " >> svn_rev.h
	@echo "$(shell if [ `svnversion -n . | sed -e 's/[a-zA-Z ]//g'` ]; then svnversion -n . | sed -e 's/[a-zA-Z ]//g'; else echo 0; fi)" >> svn_rev.h
	$(shell if [ -f main.c ]; then touch main.c; fi)
else
	$(shell if [ -f main.c ]; then touch main.c; fi)
endif

POST_MAKE_ALL_RULE_HOOK:

$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)

$(BUILDDIR) $(OBJDIR) $(LSTDIR):
	$(ECHO) Compiler Options
	$(ECHO) $(CC) -c $(CFLAGS) -I. $(IINCDIR) main.c -o main.o
	$(ECHO)
	mkdir -p $(OBJDIR)
	mkdir -p $(LSTDIR)

$(LDSCRIPT): $(SAGSCRIPT)
	$(Q)$(SAG) $(SAGSCRIPT) -o $(LDSCRIPT)

$(CPPOBJS) : $(OBJDIR)/%.o : %.cpp Makefile
	$(ECHO) Compiling $(<F)
	$(Q)$(CPPC) -c $(CPPFLAGS) -I. $(IINCDIR) $< -o $@

$(COBJS) : $(OBJDIR)/%.o : %.c Makefile
	$(ECHO) Compiling $(<F)
	$(Q)$(CC) -c $(CFLAGS) -I. $(IINCDIR) $< --depend $(basename $@).d -o $@

$(CXOBJS) : $(OBJDIR)/%.o : %.c Makefile
	$(ECHO) Compiling $(<F)
	$(Q)$(CC) -c $(CFLAGS) -fno-lto -I. $(IINCDIR) $< -o $@

$(ASMOBJS) : $(OBJDIR)/%.o : %.s Makefile
	$(ECHO) Compiling $(<F)
	$(Q)$(AS) $(ASFLAGS) -I. $(IINCDIR) $< -o $@

$(ASMXOBJS) : $(OBJDIR)/%.o : %.s Makefile
	$(ECHO) Compiling $(<F)
	$(Q)$(CC) $(ASXFLAGS) -I. $(IINCDIR) $< -o $@

# generate utest etc under linux host.
%.exe: $(OBJS)
	$(ECHO) Linking $@
	$(Q)$(CC) $(OBJS) $(LIBS) -o $@

# generate libXXX.lib w/o linking.
%.lib: $(OBJS)
	$(ECHO) Creating $@
	$(Q)$(AR) $@ $(OBJS) > /dev/null

%.axf: $(OBJS) $(LDSCRIPT)
	$(ECHO) Linking $@
	$(Q)$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.hex: %.axf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(HEX) $< --output $@

%.bin: %.axf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(BIN) $< --output $@

%.txt: %.axf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(ELF) -c $< -o $@

clean:
	$(ECHO) Cleaning
	-rm -fR .dep $(BUILDDIR)
	$(ECHO) Done

#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
