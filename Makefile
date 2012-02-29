# Hey Emacs, this is a -*- makefile -*-
#
# WinAVR Sample makefile written by Eric B. Weddington, Joerg Wunsch, et al.
# Released to the Public Domain
# Please read the make user manual!
#
# Additional material for this makefile was submitted by:
#  Tim Henigan
#  Peter Fleury
#  Reiner Patommel
#  Sander Pool
#  Frederik Rouleau
#  Markus Pfaff
#
# On command line:
#
# make all = Make software.
#
# make library = Build a library (libctbot.a) of the infrastructure components
#
# make clean = Clean out built project files.
#
# make coff = Convert ELF to AVR COFF (for use with AVR Studio 3.x or VMLAB).
#
# make extcoff = Convert ELF to AVR Extended COFF (for use with AVR Studio
#                4.07 or greater).
#
# make program = Download the hex file to the device, using avrdude.  Please
#                customize the avrdude settings below first!
#
# make filename.s = Just compile filename.c into the assembler code only
#
# To rebuild project do "make clean" then "make all".
#


# MCU name
MCU ?= atmega32

# Output format. (can be srec, ihex, binary)
FORMAT = ihex

# Target file name (without extension).
TARGET = ct-Bot

# Name of the library that contains the hardware abstraction layer
LIBRARY = libctbot.a

# Target Device, either pc or mcu, usually defined on commandline
DEVICE ?= MCU
#DEVICE ?= PC

SAVE_TEMPS ?=

MSG_DEVICE = Target device is $(DEVICE)

# List C source files here. (C dependencies are automatically generated.)
define SRCMCU
	mcu/adc.c mcu/bootloader.c mcu/bot-2-sim.c mcu/botfs-low.c mcu/cmps03.c mcu/delay.c mcu/display.c mcu/ena.c \
	mcu/i2c.c mcu/init-low.c mcu/ir-rc5.c mcu/led.c mcu/mini-fat.c mcu/mmc.c mcu/mmc-low.c mcu/motor-low.c mcu/mouse.c \
	mcu/os_scheduler.c mcu/os_thread.c mcu/sensor-low.c mcu/shift.c mcu/sp03.c mcu/spi.c mcu/srf10.c mcu/timer-low.c \
	mcu/twi.c mcu/uart.c
endef 

define SRCPC
	pc/bot-2-sim_pc.c pc/botfs_pc.c pc/botfs-low_pc.c pc/botfs-tools_pc.c pc/cmd-tools_pc.c pc/delay_pc.c pc/display_pc.c \
	pc/eeprom_pc.c pc/ena_pc.c pc/init-low_pc.c pc/ir-rc5_pc.c pc/led_pc.c pc/mini-fat_pc.c pc/mmc-emu_pc.c pc/motor-low_pc.c \
	pc/mouse_pc.c pc/os_thread_pc.c pc/sensor-low_pc.c pc/tcp-server.c pc/tcp.c pc/timer-low_pc.c pc/trace.c 
endef

define SRCHIGHLEVEL
	bot-2-bot.c botcontrol.c botfs.c command.c fifo.c init.c log.c map.c math_utils.c minilog.c mmc-vm.c motor.c pos_store.c  \
	sensor.c timer.c
endef 

define SRCLOGIC
	bot-logic/behaviour_abl.c bot-logic/behaviour_avoid_border.c bot-logic/behaviour_avoid_col.c bot-logic/behaviour_calibrate_pid.c \
	bot-logic/behaviour_calibrate_sharps.c bot-logic/behaviour_cancel_behaviour.c bot-logic/behaviour_catch_pillar.c bot-logic/behaviour_classify_objects.c \
	bot-logic/behaviour_delay.c bot-logic/behaviour_drive_area.c bot-logic/behaviour_drive_chess.c bot-logic/behaviour_drive_distance.c \ 
	bot-logic/behaviour_drive_square.c bot-logic/behaviour_drive_stack.c bot-logic/behaviour_follow_line_enhanced.c bot-logic/behaviour_follow_line.c \ 
	bot-logic/behaviour_follow_object.c bot-logic/behaviour_follow_wall.c bot-logic/behaviour_get_utilization.c bot-logic/behaviour_goto_obstacle.c \
	bot-logic/behaviour_goto_pos.c bot-logic/behaviour_goto.c bot-logic/behaviour_gotoxy.c bot-logic/behaviour_hang_on.c bot-logic/behaviour_hw_test.c \
	bot-logic/behaviour_line_shortest_way.c bot-logic/behaviour_measure_distance.c bot-logic/behaviour_olympic.c bot-logic/behaviour_pathplaning.c \
	bot-logic/behaviour_prototype.c bot-logic/behaviour_remotecall.c bot-logic/behaviour_scan.c bot-logic/behaviour_scan_beacons.c \
	bot-logic/behaviour_servo.c bot-logic/behaviour_simple.c bot-logic/behaviour_solve_maze.c bot-logic/behaviour_transport_pillar.c \
	bot-logic/behaviour_turn.c bot-logic/behaviour_ubasic.c bot-logic/bot-logic.c bot-logic/tokenizer.c bot-logic/ubasic_call.c bot-logic/ubasic_cvars.c \
	bot-logic/ubasic.c 
endef
   
SRCMAIN = ct-Bot.c

SRCUI = ui/gui.c ui/misc.c ui/rc5.c


SRCLIBRARY = $(SRCHIGHLEVEL) $(SRCUI)
ifeq ($(DEVICE),MCU)
SRCLIBRARY += $(SRCMCU)
else
SRCLIBRARY += $(SRCPC)
endif

SRCBEHAVIOUR = $(SRCMAIN) $(SRCLOGIC)


# List Assembler source files here.
# Make them always end in a capital .S.  Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC = 1st_init.S

MATH_LIB = -lm

# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
EXTRAINCDIRS = . ./include ./include/bot-logic
ifeq ($(DEVICE),MCU)
	# Assembler flags.
	#  -Wa,...:   tell GCC to pass this to the assembler.
	#  -ahlms:    create listing
	#  -gstabs:   have the assembler create line number information; note that
	#             for use in COFF files, additional information about filenames
	#             and function names needs to be present in the assembler source
	#             files -- see avr-libc docs [FIXME: not yet described there]
	# ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs 
	ASFLAGS =
	
	
	#Additional libraries.
	
	# Minimalistic printf version
	PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min
	
	# Floating point printf version (requires MATH_LIB = -lm below)
	PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt
	
	PRINTF_LIB = 
	
	# Minimalistic scanf version
	SCANF_LIB_MIN = -Wl,-u,vfscanf -lscanf_min
	
	# Floating point + %[ scanf version (requires MATH_LIB = -lm below)
	SCANF_LIB_FLOAT = -Wl,-u,vfscanf -lscanf_flt
	
	SCANF_LIB = 

	
	# Linker flags.
	#  -Wl,...:     tell GCC to pass this to linker.
	LDFLAGS = -mmcu=$(MCU)
	LDFLAGS += -Wl,--section-start=.bootloader=0x7C00
	LDFLAGS += -Wl,--whole-archive
	
	LIBS = -Wl,--no-whole-archive 
	LIBS += $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB)
	
	
	# Programming support using avrdude. Settings and variables.
	
	# Programming hardware: alf avr910 avrisp bascom bsd 
	# dt006 pavr picoweb pony-stk200 sp12 stk200 stk500
	#
	# Type: avrdude -c ?
	# to get a full listing.
	#
	AVRDUDE_PROGRAMMER = pony-stk200
	
	# com1 = serial port. Use lpt1 to connect to parallel port.
	AVRDUDE_PORT = lpt1
	
	AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
	#AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep
	
	
	# Uncomment the following if you want avrdude's erase cycle counter.
	# Note that this counter needs to be initialized first using -Yn,
	# see avrdude manual.
	#AVRDUDE_ERASE_COUNTER = -y
	
	# Uncomment the following if you do /not/ wish a verification to be
	# performed after programming the device.
	#AVRDUDE_NO_VERIFY = -V
	
	# Increase verbosity level.  Please use this when submitting bug
	# reports about avrdude. See <http://savannah.nongnu.org/projects/avrdude> 
	# to submit bug reports.
	#AVRDUDE_VERBOSE = -v -v
	
	AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
	AVRDUDE_FLAGS += $(AVRDUDE_NO_VERIFY)
	AVRDUDE_FLAGS += $(AVRDUDE_VERBOSE)
	AVRDUDE_FLAGS += $(AVRDUDE_ERASE_COUNTER)
	
	
	
	# ---------------------------------------------------------------------------

	AR = avr-ar
	AVRDUDE = avrdude
	CC = avr-gcc
	NM = avr-nm
	OBJCOPY = avr-objcopy
	OBJDUMP = avr-objdump
	RANLIB = avr-ranlib
	SIZE = avr-size
	
	# Optimization level, can be [0, 1, 2, 3, s]. 
	# 0 = turn off optimization. s = optimize for size.
	# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
	OPT = s	
	
	OUTPUT = $(TARGET).elf
else
	PTHREAD_LIB = -lpthread
	LIBS = $(PTHREAD_LIB) $(MATH_LIB)

	AR = ar
	CC = gcc
	#CC = clang
	RANLIB = ranlib
	SIZE = size
	
	# Optimization level, can be [0, 1, 2, 3, s]. 
	# 0 = turn off optimization. s = optimize for size.
	# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
	OPT = 2
	
	OUTPUT = $(TARGET)
endif



# Define programs and commands.
SHELL = sh
REMOVE = rm -f
COPY = cp

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = 


# Compiler flags.
#  -g:           generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
CFLAGS = -g3
CFLAGS += -O$(OPT)
CFLAGS += -pipe
CFLAGS += -fmessage-length=0
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -Wextra -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -MMD
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)
ifeq ($(DEVICE),MCU)
	CFLAGS += -Wconversion
endif
ifdef SAVE_TEMPS
CFLAGS += -save-temps -fverbose-asm -dA
endif

ASFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

# Flags for the library archiver (ar)
ARFLAGS = r


# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE = Size:
MSG_COFF = Converting to AVR COFF:
MSG_EXTENDED_COFF = Converting to AVR Extended COFF:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling:
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning project:
MSG_CREATING_LIBRARY = Creating library:




# Define all object files.
OBJLIBRARY = $(ASRC:.S=.o) $(SRCLIBRARY:.c=.o)
OBJBEHAVIOUR = $(SRCBEHAVIOUR:.c=.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(SRC:.c=.lst)


# Compiler flags to generate dependency files.
GENDEPFLAGS = -MP -MT"$(*F).o" -MF".dep/$(@F).d"


# Combine all necessary flags and optional flags.
# Add target processor to flags.
ifeq ($(DEVICE),MCU)
	ALL_CFLAGS = -mmcu=$(MCU) $(CFLAGS) $(GENDEPFLAGS) -D$(DEVICE)
	ALL_ASFLAGS = -mmcu=$(MCU) -x assembler-with-cpp $(ASFLAGS) -D$(DEVICE)
else
	ALL_CFLAGS = $(CFLAGS) $(GENDEPFLAGS) -D$(DEVICE)
	ALL_ASFLAGS = -x assembler-with-cpp $(ASFLAGS) -D$(DEVICE)
endif



# Default target.
all: begin gccversion build finished end

ifeq ($(DEVICE),MCU)
build: elf hex eep lss sym size
else
build: elf
endif

elf: $(OUTPUT)
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss 
sym: $(TARGET).sym

library: $(LIBRARY)

# Eye candy.
# AVR Studio 3.x does not check make's exit code but relies on
# the following magic strings to be generated by the compile job.
begin:
	@echo
	@echo $(MSG_DEVICE)
	@echo
	@echo $(MSG_BEGIN)

finished:
	@echo $(MSG_ERRORS_NONE)

end:
	@echo $(MSG_END)
	@echo


# Display size of file.
HEXSIZE = $(SIZE) --target=$(FORMAT) $(TARGET).hex
ELFSIZE = $(SIZE) -C --mcu=$(MCU) $(TARGET).elf | grep "bytes"
size:
	@if [ -f $(TARGET).elf ]; then echo; echo $(MSG_SIZE); $(ELFSIZE); echo; fi


# Display compiler version information.
gccversion : 
	@$(CC) --version


# Program the device.  
program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)



# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
--change-section-address .data-0x800000 \
--change-section-address .bss-0x800000 \
--change-section-address .noinit-0x800000 \
--change-section-address .eeprom-0x810000 


coff: $(TARGET).elf
	@echo
	@echo $(MSG_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-avr $< $(TARGET).cof


extcoff: $(TARGET).elf
	@echo
	@echo $(MSG_EXTENDED_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-ext-avr $< $(TARGET).cof


$(LIBRARY): $(OBJLIBRARY)
	@echo
	@echo $(MSG_CREATING_LIBRARY) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	@echo
	@echo $(MSG_EEPROM) $@
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	$(NM) -n $< > $@



# Link: create ELF output file from object files.
.SECONDARY : $(OUTPUT)
.PRECIOUS : $(OBJBEHAVIOUR)
$(OUTPUT): $(OBJBEHAVIOUR) $(LIBRARY)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) --output $@ $(LDFLAGS) $^ $(LIBS)


# Compile: create object files from C source files.
%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@ 


# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
%.o : %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(CC) -c $(ALL_ASFLAGS) $< -o $@



# Target: clean project.
clean: begin clean_list finished end

clean_list :
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET)
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).eep
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).cof
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).a90
	$(REMOVE) $(TARGET).sym
	$(REMOVE) $(TARGET).lnk
	$(REMOVE) $(TARGET).lss
	$(REMOVE) $(LIBRARY)
	$(REMOVE) $(OBJBEHAVIOUR)
	$(REMOVE) $(ASRC:.S=.o)
	$(REMOVE) $(SRCHIGHLEVEL:.c=.o)
	$(REMOVE) $(SRCUI:.c=.o)
	$(REMOVE) $(SRCMCU:.c=.o)
	$(REMOVE) $(SRCPC:.c=.o)
	$(REMOVE) $(LST)
	$(REMOVE) $(notdir $(SRCBEHAVIOUR:.c=.s))
	$(REMOVE) $(notdir $(SRCHIGHLEVEL:.c=.s))
	$(REMOVE) $(notdir $(SRCUI:.c=.s))
	$(REMOVE) $(notdir $(SRCMCU:.c=.s))
	$(REMOVE) $(notdir $(SRCPC:.c=.s))
	$(REMOVE) $(notdir $(SRCBEHAVIOUR:.c=.i))
	$(REMOVE) $(notdir $(SRCHIGHLEVEL:.c=.i))
	$(REMOVE) $(notdir $(SRCUI:.c=.i))
	$(REMOVE) $(notdir $(SRCMCU:.c=.i))
	$(REMOVE) $(notdir $(SRCPC:.c=.i))
	$(REMOVE) $(SRCBEHAVIOUR:.c=.d)
	$(REMOVE) $(SRCHIGHLEVEL:.c=.d)
	$(REMOVE) $(SRCUI:.c=.d)
	$(REMOVE) $(SRCMCU:.c=.d)
	$(REMOVE) $(SRCPC:.c=.d)
	$(REMOVE) .dep/*


# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all begin finish end size gccversion \
build elf hex eep lss sym coff extcoff \
clean clean_list program


# you need to erase first before loading the program.
# load (program) the software into the eeprom:
load: avrledtest.hex
        uisp -dlpt=/dev/parport0 --erase  -dprog=dapa
        uisp -dlpt=/dev/parport0 --upload if=$(TARGET).hex -dprog=dapa  -v=3 --hash=32
