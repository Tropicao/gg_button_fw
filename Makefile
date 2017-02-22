# Source : 	* https://gist.github.com/edwardhotchkiss/9378977 (last seen
# 14/02/2017)
# 			* http://codeandlife.com/data/Makefile (last seen 14/02/2017)
#
# To use this Makefile, please install avr-gcc and utils for avr (avr-objcopy,
# arv-size, avrdude)

CC			= avr-gcc
OBJCOPY		= avr-objcopy
DUDE		= avrdude

DEVICE		= attiny45
PROGRAMMER 	= stk500v1
PORT		= /dev/ttyACM0
BAUDRATE	= 19200

CFLAGS 		= -Wall -Os -Iusbdrv -mmcu=$(DEVICE)
OBJFLAGS	= -j .text -j .data -O ihex

DUDEFLAGS	= -p $(DEVICE) -c $(PROGRAMMER) -P $(PORT) -b $(BAUDRATE) -v -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m

OBJECTS		= usbdrv/usbdrv.o usbdrv/oddebug.o usbdrv/usbdrvasm.o main.o
CMDLINE		= ggbuttontest

PROGRAM		= main.hex

all: clean $(PROGRAM) $(CMDLINE)
#all : $(CMDLINE)

$(CMDLINE): ggbuttontest.c
	gcc -DDEBUG -I ./libusb/include \
		-I /usr/local/lib \
	   	-L ./libusb/lib/gcc \
		-L /usr/local/lib \
		-O -Wall -lusb -luv $< -o $@
%.hex: %.elf
	$(OBJCOPY) $(OBJFLAGS) $< $@

main.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

led.elf: led.o
	$(CC) -Wall -Os -mmcu=$(DEVICE) led.o -o $@

$(OBJECTS): usbdrv/usbconfig.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

flash:$(PROGRAM)
	$(DUDE) $(DUDEFLAGS) -U flash:w:$<

clean:
	$(RM) *.o *.hex *.elf usbdrv/*.o $(CMDLINE)
