
# User defined values
TARGETNAME = u8m2
MCU:=atmega168
F_CPU:=16000000
WORKDIR:=.
U8GM2DIR:=../u8glib
MPU6050DIR:=../mpu

# Type: "avrdude -c ?" to get a full listing.
AVRDUDE_PROGRAMMER := avrispmkii
# com1 = serial port. Use lpt1 to connect to parallel port.
AVRDUDE_PORT := usb

# Replace standard build tools by avr tools
CC = avr-gcc
AR  = @avr-ar

# Compile all .c files in all directories
SRC = $(shell ls $(WORKDIR)/*.c 2>/dev/null)
SRC += $(shell ls $(U8GM2DIR)/*.c 2>/dev/null)
SRC += $(shell ls $(MPU6050)/*.c 2>/dev/null)

# Flags for the linker and the compiler
COMMON_FLAGS = -DF_CPU=$(F_CPU) -mmcu=$(MCU) $(DOGDEFS)
COMMON_FLAGS += -I$(WORKDIR) -I$(U8GM2DIR) -I$(MPU6050DIR)
COMMON_FLAGS += -g -Os -Wall -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
COMMON_FLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections
COMMON_FLAGS += -Wl,--relax -mcall-prologues
CFLAGS = $(COMMON_FLAGS) -std=gnu99    

OBJ = $(SRC:.c=.o)

.SUFFIXES: .elf .hex .dis

# Targets
.PHONY: all
all: $(TARGETNAME).dis $(TARGETNAME).hex
	avr-size $(TARGETNAME).elf

.PHONY: upload
upload: $(TARGETNAME).dis $(TARGETNAME).hex
	avrdude -F -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -v -v -U flash:w:$(TARGETNAME).hex
	avr-size $(TARGETNAME).elf

.PHONY: clean
clean:
	$(RM) $(TARGETNAME).hex $(TARGETNAME).elf $(TARGETNAME).a $(TARGETNAME).dis $(OBJ)

# implicit rules
.elf.hex:
	avr-objcopy -O ihex -R .eeprom $< $@

# explicit rules
$(TARGETNAME).elf: $(TARGETNAME).a($(OBJ)) 
	$(LINK.o) $(COMMON_FLAGS) $(TARGETNAME).a $(LOADLIBES) $(LDLIBS) -o $@

$(TARGETNAME).dis: $(TARGETNAME).elf
	avr-objdump -S $< > $@

