P2CC=fastspin

TARGET=junkbasic

SRCS=\
edit.c \
editbuf.c \
generate.c \
parse.c \
scan.c \
symbols.c \
system.c \
osint_posix.c

HDRS=\
compile.h \
image.h \
system.h \
types.h

$(TARGET):	$(SRCS) $(HDRS) Makefile
	$(CC) -DMAC -o $@ $(SRCS)

$(TARGET).p2:	$(SRCS) $(HDRS) Makefile
	$(P2CC) -DPROPELLER -2b -o $@ $(SRCS)

run:	$(TARGET)
	./junkbasic
	
p2:		$(TARGET).p2

run-p2:		$(TARGET).p2
	loadp2 -b 230400 $(TARGET).p2 -t
    
clean:
	rm -f $(TARGET)
