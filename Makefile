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
CMDLINE		= usbtest



all: clean main.hex flash

$(CMDLINE): usbtest.c
	gcc -I ./libusb/include -L ./libusb/lib/gcc -O -Wall usbtest.c -o usbtest -lusb

%.hex: %.elf
	$(OBJCOPY) $(OBJFLAGS) $< $@

main.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(OBJECTS): usbdrv/usbconfig.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@



flash:main.hex
	$(DUDE) $(DUDEFLAGS) -U flash:w:$<

clean:
	$(RM) *.o *.hex *.elf usbdrv/*.o usbtest
