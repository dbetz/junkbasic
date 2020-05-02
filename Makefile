P2CC=fastspin

TARGET=junkbasic

SRCS=\
compile.c \
debug.c \
edit.c \
generate.c \
parse.c \
scan.c \
symbols.c \
system.c \
vmdebug.c \
vmint.c \
osint_posix.c

HDRS=\
compile.h \
image.h \
system.h \
types.h

CFLAGS=-Wall -Wno-unused-function

$(TARGET):	$(SRCS) $(HDRS) Makefile
	$(CC) -DMAC -DLOAD_SAVE $(CFLAGS) -o $@ $(SRCS)

$(TARGET).p2:	$(SRCS) $(HDRS) Makefile
	$(P2CC) -DPROPELLER -DLOAD_SAVE -2b -o $@ $(SRCS)

run:	$(TARGET)
	./junkbasic
	
p2:		$(TARGET).p2

run-p2:		$(TARGET).p2
	loadp2 -b 230400 -9 . $(TARGET).p2 -t
    
clean:
	rm -f $(TARGET) *.pasm *.p2asm
