MCU=atmega8515
DUDE_MCU=m8515
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-Wall -g -Os -mmcu=${MCU}
TARGET=main
SRCS=ledivilkku.c animation.c led.c
PROGRAMMER=usbtiny
LFUSE=0xe4
HFUSE=0xd9

all:
	mkdir -p bin
	${CC} ${CFLAGS} -o ./bin/${TARGET}.bin ${SRCS}
	${OBJCOPY} -j .text -j .data -O ihex ./bin/${TARGET}.bin ./bin/${TARGET}.hex

test:
	avrdude -p ${DUDE_MCU} -c ${PROGRAMMER}

size:
	avr-size -C ./bin/${TARGET}.bin --mcu=${MCU}

fuse:
	avrdude -p ${DUDE_MCU} -c ${PROGRAMMER} -U lfuse:w:${LFUSE}:m -U hfuse:w:${HFUSE}:m

flash:
	avrdude -p ${DUDE_MCU} -c ${PROGRAMMER} -U flash:w:./bin/${TARGET}.hex:i

clean:
	rm -f *.bin *.hex
