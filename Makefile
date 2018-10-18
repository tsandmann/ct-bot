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
MCU ?= atmega1284p

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
WERROR ?=
WCONVERSION ?=
BUILD_TARGET ?=

MSG_DEVICE = Target device is $(DEVICE)

# List C source files here. (C dependencies are automatically generated.)
define SRCMCU
	mcu/adc.c mcu/bootloader.c mcu/bot-2-linux.c mcu/bot-2-sim.c mcu/cmps03.c mcu/cppsupport.cpp mcu/delay.c \
	mcu/display.c mcu/ena.c mcu/i2c.c mcu/init-low.c mcu/ir-rc5.c mcu/led.c \
	mcu/mmc.c mcu/motor-low.c mcu/mouse.c mcu/os_scheduler.c mcu/os_thread.c mcu/sdcard_wrapper.cpp mcu/sdcard.cpp mcu/sensor-low.c mcu/shift.c \
	mcu/sp03.c mcu/srf10.c mcu/timer-low.c mcu/twi.c mcu/uart.c \
	mcu/SdFat/Print.cpp mcu/SdFat/FatLib/FatFile.cpp mcu/SdFat/FatLib/FatFileLFN.cpp mcu/SdFat/FatLib/FatFilePrint.cpp mcu/SdFat/FatLib/FatFileSFN.cpp \
	mcu/SdFat/FatLib/FatVolume.cpp mcu/SdFat/FatLib/FmtNumber.cpp
endef 

define SRCPC
	pc/bot-2-atmega_pc.c pc/bot-2-sim_pc.c \
	pc/cmd-tools_pc.c pc/delay_pc.c pc/display_pc.c pc/ena_pc.c pc/init-low_pc.c \
	pc/ir-rc5_pc.c pc/led_pc.c pc/motor-low_pc.c pc/mouse_pc.c \
	pc/os_thread_pc.c pc/sdfat_fs_pc.c pc/sensor-low_pc.c pc/tcp-server.c pc/tcp.c pc/timer-low_pc.c pc/trace.c \
	pc/uart-test_pc.c pc/uart_pc.c
endef

define SRCHIGHLEVEL
	bot-2-bot.c botcontrol.c command.c fifo.c init.c log.c map.c math_utils.c \
	minilog.c motor.c pos_store.c sensor.c timer.c
endef 

define SRCLOGIC
	bot-logic/behaviour_abl.c bot-logic/behaviour_avoid_border.c bot-logic/behaviour_avoid_col.c \
	bot-logic/behaviour_calibrate_pid.c bot-logic/behaviour_calibrate_sharps.c \
	bot-logic/behaviour_cancel_behaviour.c bot-logic/behaviour_catch_pillar.c \
	bot-logic/behaviour_classify_objects.c bot-logic/behaviour_delay.c bot-logic/behaviour_drive_area.c \
	bot-logic/behaviour_drive_chess.c bot-logic/behaviour_drive_distance.c \
	bot-logic/behaviour_drive_neuralnet.c bot-logic/behaviour_drive_square.c \
	bot-logic/behaviour_drive_stack.c bot-logic/behaviour_follow_line.c \
	bot-logic/behaviour_follow_line_enhanced.c bot-logic/behaviour_follow_object.c \
	bot-logic/behaviour_follow_wall.c bot-logic/behaviour_get_utilization.c bot-logic/behaviour_goto.c \
	bot-logic/behaviour_goto_obstacle.c bot-logic/behaviour_goto_pos.c bot-logic/behaviour_gotoxy.c \
	bot-logic/behaviour_hang_on.c bot-logic/behaviour_hw_test.c bot-logic/behaviour_line_shortest_way.c \
	bot-logic/behaviour_measure_distance.c bot-logic/behaviour_neuralnet.c bot-logic/behaviour_olympic.c \
	bot-logic/behaviour_pathplanning.c bot-logic/behaviour_prototype.c bot-logic/behaviour_remotecall.c \
	bot-logic/behaviour_scan.c bot-logic/behaviour_scan_beacons.c bot-logic/behaviour_servo.c \
	bot-logic/behaviour_simple.c bot-logic/behaviour_solve_maze.c bot-logic/behaviour_test_encoder.c \
	bot-logic/behaviour_transport_pillar.c bot-logic/behaviour_turn.c bot-logic/behaviour_turn_test.c \
	bot-logic/behaviour_ubasic.c bot-logic/bot-logic.c bot-logic/network.c bot-logic/tokenizer.c \
	bot-logic/ubasic.c bot-logic/ubasic_call.c bot-logic/ubasic_cvars.c
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
ASRC = 

MATH_LIB = -lm

# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
EXTRAINCDIRS = . ./include ./include/bot-logic ./mcu/SdFat
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
	
	CFLAGS = -ffunction-sections -fdata-sections
	CXXFLAGS = -fno-exceptions -fno-threadsafe-statics -felide-constructors -ffunction-sections -fdata-sections
	
	
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
	LDFLAGS += -Wl,--section-start=.bootloader=0x1F800
	LDFLAGS += -Wl,--whole-archive -Wl,--gc-sections
	
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
	CXX = avr-g++
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

ifdef BUILD_TARGET
	AR = $(BUILD_TARGET)-ar
	CC = $(BUILD_TARGET)-gcc
	CXX = $(BUILD_TARGET)-g++
	RANLIB = $(BUILD_TARGET)-ranlib
	SIZE = $(BUILD_TARGET)-size
else
	AR = ar
	CC = gcc
	CXX = g++
	RANLIB = ranlib
	SIZE = size
endif
	
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

# Compiler flag to set the C/C++ Standard level.
CSTANDARD = -std=gnu11
CXXSTANDARD = -std=gnu++1y

# Compiler flags.
#  -g:           generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
CFLAGS = -g
CFLAGS += -O$(OPT)
CFLAGS += -fmessage-length=0
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -Wextra -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -MMD
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)
CFLAGS += -Wshadow -Wformat=2
ifeq ($(DEVICE),MCU)
ifeq ($(WCONVERSION),1)
CFLAGS += -Wconversion
endif
else
CFLAGS += -Wdouble-promotion
endif
ifeq ($(BUILD_TARGET),arm-linux-gnueabihf)
CFLAGS += -mcpu=cortex-a7 -mtune=cortex-a7 -mfloat-abi=hard -mfpu=vfpv4
endif
ifeq ($(BUILD_TARGET),armv8l-linux-gnueabihf)
CFLAGS += -mcpu=cortex-a53 -mtune=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8
endif
ifeq ($(BUILD_TARGET),x86_64-w64-mingw32)
LIBS += -lws2_32
endif
ifdef SAVE_TEMPS
CFLAGS += -save-temps -fverbose-asm -dA
endif
ifeq ($(WERROR),1)
CFLAGS += -Werror
endif

CXXFLAGS += -g
CXXFLAGS += -O$(OPT)
CXXFLAGS += -fmessage-length=0
CXXFLAGS += -Wall -Wextra -Wmissing-declarations
CXXFLAGS += -MMD
CXXFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CXXFLAGS += $(CXXSTANDARD)
CXXFLAGS += -Wlogical-op -Wshadow -Wformat=2 -Wold-style-cast -Wuseless-cast
ifeq ($(DEVICE),PC)
CXXFLAGS += -Wdouble-promotion
endif
ifdef SAVE_TEMPS
CXXFLAGS += -save-temps -fverbose-asm -dA
endif
ifeq ($(WERROR),1)
CXXFLAGS += -Werror
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
OBJLIBRARY_ = $(patsubst %.S,%.o,$(ASRC)) $(patsubst %.c,%.o,$(SRCLIBRARY)) 
OBJLIBRARY = $(patsubst %.cpp,%.o,$(OBJLIBRARY_)) 
OBJBEHAVIOUR = $(SRCBEHAVIOUR:.c=.o)


# Compiler flags to generate dependency files.
GENDEPFLAGS = -MP -MT"$(*F).o" -MF".dep/$(@F).d"


# Combine all necessary flags and optional flags.
# Add target processor to flags.
ifeq ($(DEVICE),MCU)
	ALL_CFLAGS = -mmcu=$(MCU) $(CFLAGS) $(GENDEPFLAGS) -D$(DEVICE)
	ALL_CXXFLAGS = -mmcu=$(MCU) $(CXXFLAGS) $(GENDEPFLAGS) -D$(DEVICE)
	ALL_ASFLAGS = -mmcu=$(MCU) -x assembler-with-cpp $(ASFLAGS) -D$(DEVICE)
else
	ALL_CFLAGS = $(CFLAGS) $(GENDEPFLAGS) -D$(DEVICE)
	ALL_CXXFLAGS = $(CXXFLAGS) $(GENDEPFLAGS) -D$(DEVICE)
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
	@$(CXX) --version


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
	$(CXX) --output $@ $(LDFLAGS) $^ $(LIBS)


# Compile: create object files from C source files.
%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@ 

%.o : %.cpp
	@echo
	@echo $(MSG_COMPILING) $<
	$(CXX) -c $(ALL_CXXFLAGS) $< -o $@ 


# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@
	
%.s : %.cpp
	$(CXX) -S $(ALL_CXXFLAGS) $< -o $@


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
	$(REMOVE) $(OBJBEHAVIOUR) $(OBJLIBRARY) $(LIBRARY)
	$(REMOVE) .dep/*


# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)


# Listing of phony targets.
.PHONY : all begin finish end size gccversion \
build elf hex eep lss sym coff extcoff \
clean clean_list program
