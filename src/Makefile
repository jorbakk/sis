###### Site-specific configuration parameters
###### edit here:
LIBGR_DIR = /usr/include/gr
BIN_DIR = /usr/local/bin
MAN_DIR = /usr/local/man/man1

######

LIB_TIFF = tiff
CC = gcc
#CFLAGS = -Wall -g
CFLAGS = -Wall -O2
LDFLAGS = -lm
OBJS = sis.o tiff.o tga.o algorith.o get_opt.o

sis: $(OBJS)
	$(CC) -o sis $(OBJS) -l$(LIB_TIFF) $(LDFLAGS)
get_opt.o: get_opt.c defines.h sis.h
	$(CC) -c $(CFLAGS) get_opt.c
algorith.o: algorith.c sis.h defines.h
	$(CC) -c $(CFLAGS) algorith.c
tiff.o: tiff.c tiff.h sis.h defines.h
	$(CC) -c $(CFLAGS) -I$(LIBGR_DIR) tiff.c
tga.o: tga.c tga.h sis.h defines.h
	$(CC) -c $(CFLAGS) tga.c
sis.o: sis.c tiff.h sis.h defines.h
	$(CC) -c $(CFLAGS) sis.c

clean:
	rm *.o; rm core

install:
	install -s sis $(BIN_DIR)
	install -m 0644 sis.1 $(MAN_DIR)
