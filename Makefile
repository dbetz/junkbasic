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
	$(P2CC) -DPROPELLER -o $@ $(SRCS) strncasecmp.c

run:	$(TARGET)
	./junkbasic
	
p2:		$(TARGET).p2
    
clean:
	rm -f $(TARGET)
