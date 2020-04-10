TARGET=junkbasic

SRCS=\
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
	$(CC) -ferror-limit=100 -DMAC -o $@ $(SRCS)

run:	$(TARGET)
	./junkbasic
    
clean:
	rm -f $(TARGET)
