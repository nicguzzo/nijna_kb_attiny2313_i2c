# Name: Makefile
# Author: <insert your name here>
# Copyright: <insert your copyright message here>
# License: <insert your license reference here>

# DEVICE ....... The AVR device you compile for
# CLOCK ........ Target AVR clock rate in Hertz
# OBJECTS ...... The object files created from your source files. This list is
#                usually the same as the list of source files with suffix ".o".
# PROGRAMMER ... Options to avrdude which define the hardware you use for
#                uploading to the AVR and the interface where this hardware
#                is connected.
# FUSES ........ Parameters for avrdude to flash the fuses appropriately.

DEVICE     = attiny2313
CLOCK      = 8000000
#CLOCK      = 1000000
PROGRAMMER = -c usbasp 
OBJECTS    = main.o
#FUSES      = -U lfuse:w:0x64:m -U hfuse:w:0xdd:m -U efuse:w:0xff:m
FUSES      = -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m


model = ${MODEL}
flags = -D${model}

######################################################################
######################################################################
BINPATH =
AVRDUDE_PATH=
AVRDUDE = $(AVRDUDE_PATH)avrdude $(PROGRAMMER) -p attiny2313
COMPILE = $(BINPATH)avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) ${flags}
SIZE    = $(BINPATH)avr-size main_${model}.elf

# symbolic targets:
all:	main.hex size

.c.o:
	$(COMPILE) -c $< -o $@
.cpp.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:main_${model}.hex:i
size:
	$(SIZE)
fuse:
	$(AVRDUDE) $(FUSES)

install: flash fuse

clean:
	rm -f *.hex *.elf $(OBJECTS)

# file targets:
main.elf: $(OBJECTS)
	$(COMPILE) -o main_${model}.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main_${model}.elf main_${model}.hex
# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm:	main.elf
	avr-objdump -d main_${model}.elf

cpp:
	$(COMPILE) -E main.c