# Andes makefile scripts and rules.

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

# Data flags
OPT += -fno-strict-aliasing -fno-builtin --short-enums

# Garbage collection
ifeq ($(USE_LINK_GC),yes)
  OPT += -ffunction-sections -fdata-sections -fno-common
  LDOPT := ,--gc-sections
else
  LDOPT := ,--no-gc-sections
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

# CMODEL
ifneq ($(findstring arm-none-eabi-,$(TRGT)),) # ARM
CMODEL := -mthumb
else ifneq ($(findstring nds32le-elf-,$(TRGT)),) # NDS
CMODEL := -mcmodel=small
else ifneq ($(findstring riscv-nuclei-elf-,$(TRGT)),) # RISC-V
CMODEL := -mcmodel=medlow
endif

# Add `-fno-delete-null-pointer-checks` flag if the compiler supports it.
# GCC assumes that programs cannot safely dereference null pointers,
# and that no code or data element resides there.
# However, 0x0 is the vector table memory location, so the test must not be removed.
ifeq ($(shell $(CC) -fno-delete-null-pointer-checks -E - 2>/dev/null >/dev/null </dev/null ; echo $$?),0)
OPT += -fno-delete-null-pointer-checks
endif

ifeq ($(shell echo | $(CC) -E -dM - | grep __NDS32_EXT_IFC__ > /dev/null && echo IFC),IFC)
ifeq ($(USE_IFC),no)
OPT += -mno-ifc -DCONFIG_NO_NDS32_EXT_IFC
endif
else
ifneq ($(USE_IFC),no)
$(error this toolchain do not support IFC extension)
endif
endif

ifeq ($(shell echo | $(CC) -E -dM - | grep __NDS32_EXT_EX9__ > /dev/null && echo EX9),EX9)
ifeq ($(USE_EX9),no)
OPT += -mno-ex9 -DCONFIG_NO_NDS32_EXT_EX9
endif
else
ifneq ($(USE_EX9),no)
$(error this toolchain do not support EX9 extension)
endif
endif

# Process stack size
ifeq ($(USE_PROCESS_STACKSIZE),)
  LDOPT := $(LDOPT),--defsym=__process_stack_size__=0x400
else
  LDOPT := $(LDOPT),--defsym=__process_stack_size__=$(USE_PROCESS_STACKSIZE)
endif

# Exceptions stack size
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  LDOPT := $(LDOPT),--defsym=__main_stack_size__=0x400
else
  LDOPT := $(LDOPT),--defsym=__main_stack_size__=$(USE_EXCEPTIONS_STACKSIZE)
endif

# Output directory and files
ifeq ($(BUILDDIR),)
  BUILDDIR = build
endif
ifeq ($(BUILDDIR),.)
  BUILDDIR = build
endif
ifeq ($(LDSCRIPT),)
OUTFILES = $(BUILDDIR)/lib$(PROJECT).a
else
OUTFILES = $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PROJECT).hex \
           $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT).dmp \
           $(BUILDDIR)/$(PROJECT).list

ifdef SREC
OUTFILES += $(BUILDDIR)/$(PROJECT).srec
endif
endif
ifeq ($(MCU),host)
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
ASMOBJS   = $(addprefix $(OBJDIR)/, $(notdir $(ASMSRC:.S=.o)))
ASMXOBJS  = $(addprefix $(OBJDIR)/, $(notdir $(ASMXSRC:.S=.o)))
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
ifeq ($(MCU),host)
MCFLAGS   = #-m32
else ifeq ($(MCU),rv32imac)
MCFLAGS   = -march=$(MCU) -mabi=ilp32 $(CMODEL)
else ifeq ($(MCU),rv32emac)
MCFLAGS   = -march=$(MCU) -mabi=ilp32e $(CMODEL)
else
MCFLAGS   = -mcpu=$(MCU) $(CMODEL)
endif
ODFLAGS	  = -h -x --syms -S
ASFLAGS   = $(MCFLAGS) $(OPT) -Wa,-amhls=$(LSTDIR)/$(notdir $(<:.S=.lst)) $(ADEFS)
ASXFLAGS  = $(MCFLAGS) $(OPT) -Wa,-amhls=$(LSTDIR)/$(notdir $(<:.S=.lst)) $(ADEFS)
CFLAGS    = $(MCFLAGS) $(OPT) $(COPT) $(CWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.c=.lst)) $(DEFS)
CPPFLAGS  = $(MCFLAGS) $(OPT) $(CPPOPT) $(CPPWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.cpp=.lst)) $(DEFS)
LDFLAGS   = $(MCFLAGS) $(OPT) -nostartfiles $(LLIBDIR) -Wl,-Map=$(BUILDDIR)/$(PROJECT).map,--cref,--no-warn-mismatch,--library-path=$(RULESPATH),--script=$(LDSCRIPT)$(LDOPT)
# Use startfiles for rv32
ifeq ($(LDSCRIPT),elf32lriscv.x)
LDFLAGS   = $(MCFLAGS) $(OPT) $(LLIBDIR) -Wl,-Map=$(BUILDDIR)/$(PROJECT).map,--cref,--no-warn-mismatch,--library-path=$(RULESPATH)
endif

# Generate dependency information
ASFLAGS  += -MD -MP -MF .dep/$(@F).d
CFLAGS   += -MD -MP -MF .dep/$(@F).d
CPPFLAGS += -MD -MP -MF .dep/$(@F).d

# Paths where to search for sources
VPATH     = $(SRCPATHS)

#
# Makefile rules
#

all: PRE_MAKE_ALL_RULE_HOOK $(OBJS) $(OUTFILES) POST_MAKE_ALL_RULE_HOOK
	echo "all"

PRE_MAKE_ALL_RULE_HOOK:
	touch main.c
	@echo -n "#define SVN_REV " > ../../inc/svn_rev.h
	@echo "\"$(shell svnversion -n .)\"" >> ../../inc/svn_rev.h
	@echo -n "#define SVN_REV_NUM " >> ../../inc/svn_rev.h
	@echo "$(shell if [ `svnversion -n . | sed -e 's/[a-zA-Z ]//g'` ]; then svnversion -n . | sed -e 's/[a-zA-Z ]//g'; else echo 0; fi)" >> ../../inc/svn_rev.h

POST_MAKE_ALL_RULE_HOOK:

$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)

$(BUILDDIR) $(OBJDIR) $(LSTDIR):
ifneq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO) Compiler Options
	$(ECHO) $(CC) -c $(CFLAGS) -I. $(IINCDIR) main.c -o main.o
	$(ECHO)
endif
	mkdir -p $(OBJDIR)
	mkdir -p $(LSTDIR)

$(LDSCRIPT): $(SAGSCRIPT)
	$(Q)$(SAG) $(SAGSCRIPT) -o $(LDSCRIPT)

$(CPPOBJS) : $(OBJDIR)/%.o : %.cpp Makefile
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO)
	$(CPPC) -c $(CPPFLAGS) -I. $(IINCDIR) $< -o $@
else
	$(ECHO) Compiling $(<F)
	$(Q)$(CPPC) -c $(CPPFLAGS) -I. $(IINCDIR) $< -o $@
endif

$(COBJS) : $(OBJDIR)/%.o : %.c Makefile
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO)
	$(CC) -c $(CFLAGS) -I. $(IINCDIR) $< -o $@
else
	$(ECHO) Compiling $(<F)
	$(Q)$(CC) -c $(CFLAGS) -I. $(IINCDIR) $< -o $@
endif

$(CXOBJS) : $(OBJDIR)/%.o : %.c Makefile
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO)
	$(CC) -c $(CFLAGS) -fno-lto -I. $(IINCDIR) $< -o $@
else
	$(ECHO) Compiling $(<F)
	$(Q)$(CC) -c $(CFLAGS) -fno-lto -I. $(IINCDIR) $< -o $@
endif

$(ASMOBJS) : $(OBJDIR)/%.o : %.S Makefile
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO)
	$(AS) -c $(ASFLAGS) -I. $(IINCDIR) $< -o $@
else
	$(ECHO) Compiling $(<F)
	$(Q)$(AS) -c $(ASFLAGS) -I. $(IINCDIR) $< -o $@
endif

$(ASMXOBJS) : $(OBJDIR)/%.o : %.S Makefile
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO)
	$(CC) -c $(ASXFLAGS) -I. $(IINCDIR) $< -o $@
else
	$(ECHO) Compiling $(<F)
	$(Q)$(CC) -c $(ASXFLAGS) -I. $(IINCDIR) $< -o $@
endif

# generate utest etc under linux host.
%.exe: $(OBJS)
ifeq ($(USE_VERBOSE_COMPILE),yes)
	$(ECHO)
	$(CC) $(OBJS) $(LIBS) -o $@
else
	$(ECHO) Linking $@
	$(Q)$(CC) $(OBJS) $(LIBS) -o $@
endif

# generate libXXX.a w/o linking.
%.a: $(OBJS)
	$(ECHO) Creating $@
	$(Q)$(AR) $(ARFLAGS) $@ $(OBJS) > /dev/null

%.elf: $(OBJS) $(LDSCRIPT)
	$(ECHO) Linking $@
	$(Q)$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.hex: %.elf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(HEX) $< $@

%.bin: %.elf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(BIN) $< $@

%.dmp: %.elf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(OD) $(ODFLAGS) $< > $@
	$(ECHO)
	$(Q)$(SZ) $<

%.list: %.elf $(LDSCRIPT)
	$(ECHO) Creating $@
	$(Q)$(OD) -S $< > $@
	$(ECHO) Done

clean:
	$(ECHO) Cleaning
	-rm -fR .dep $(BUILDDIR)
	$(ECHO) Done

#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
