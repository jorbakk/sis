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

# LIB_TIFF = tiff
CC = gcc
CFLAGS = -Wall -g
# CFLAGS = -Wall -O2
LDFLAGS = -lm
SRC = .
BUILD = build
# OBJS = $(BUILD)/main.o $(BUILD)/stbimg.o $(BUILD)/tiff.o $(BUILD)/algorithm.o $(BUILD)/get_opt.o
OBJS = $(BUILD)/main.o $(BUILD)/stbimg.o $(BUILD)/algorithm.o $(BUILD)/get_opt.o

# $(BUILD)/sis: build_dir $(OBJS)
# 	$(CC) -o $(BUILD)/sis $(OBJS) -l$(LIB_TIFF) $(LDFLAGS)
$(BUILD)/sis: build_dir $(OBJS)
	$(CC) -o $(BUILD)/sis $(OBJS) $(LDFLAGS)
$(BUILD)/get_opt.o: $(SRC)/get_opt.c $(SRC)/sis.h
	$(CC) -c -o $(BUILD)/get_opt.o $(CFLAGS) $(SRC)/get_opt.c
$(BUILD)/algorithm.o: $(SRC)/algorithm.c $(SRC)/sis.h
	$(CC) -c -o $(BUILD)/algorithm.o $(CFLAGS) $(SRC)/algorithm.c
$(BUILD)/stbimg.o: $(SRC)/stbimg.c $(SRC)/stbimg.h $(SRC)/sis.h
	$(CC) -c -o $(BUILD)/stbimg.o $(CFLAGS) $(SRC)/stbimg.c
# $(BUILD)/tiff.o: $(SRC)/tiff.c $(SRC)/tiff.h $(SRC)/sis.h
# 	$(CC) -c -o $(BUILD)/tiff.o $(CFLAGS) $(SRC)/tiff.c
# $(BUILD)/main.o: $(SRC)/main.c $(SRC)/stbimg.h $(SRC)/tiff.h $(SRC)/sis.h
$(BUILD)/main.o: $(SRC)/main.c $(SRC)/stbimg.h $(SRC)/sis.h
	$(CC) -c -o $(BUILD)/main.o $(CFLAGS) $(SRC)/main.c

clean:
	rm -rf $(BUILD)
build_dir:
	@mkdir -p $(BUILD)
install:
	install -s $(BUILD)/sis $(BIN_DIR)
	install -m 0644 doc/sis.1 $(MAN_DIR)
