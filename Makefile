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

run:	$(TARGET)
	./junkbasic
    
clean:
	rm -f $(TARGET)
