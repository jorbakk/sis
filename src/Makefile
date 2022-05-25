#  Copyright 1995, 2021 Jörg Bakker
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy of
#  this software and associated documentation files (the "Software"), to deal in
#  the Software without restriction, including without limitation the rights to
#  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
#  of the Software, and to permit persons to whom the Software is furnished to do
#  so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.


###### Site-specific configuration parameters
###### edit here:
BIN_DIR = /usr/local/bin
MAN_DIR = /usr/local/man/man1

######

LIB_TIFF = tiff
CC = gcc
CFLAGS = -Wall -g
# CFLAGS = -Wall -O2
LDFLAGS = -lm
OBJS = sis.o stbimg.o tiff.o algorithm.o get_opt.o

sis: $(OBJS)
	$(CC) -o sis $(OBJS) -l$(LIB_TIFF) $(LDFLAGS)
get_opt.o: get_opt.c defines.h sis.h
	$(CC) -c $(CFLAGS) get_opt.c
algorithm.o: algorithm.c sis.h defines.h
	$(CC) -c $(CFLAGS) algorithm.c
stbimg.o: stbimg.c stbimg.h sis.h defines.h
	$(CC) -c $(CFLAGS) stbimg.c
tiff.o: tiff.c tiff.h sis.h defines.h
	$(CC) -c $(CFLAGS) tiff.c
sis.o: sis.c stbimg.h tiff.h sis.h defines.h
	$(CC) -c $(CFLAGS) sis.c

clean:
	rm -f *.o sis core

install:
	install -s sis $(BIN_DIR)
	install -m 0644 sis.1 $(MAN_DIR)
