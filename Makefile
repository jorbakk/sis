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
S = .
S3 = $(S)/3rd-party
B = build

# LIB_TIFF = tiff
# CC = gcc
# CFLAGS = -Wall -g -I$(S3) -DNANOVG_USE_GLEW -DNANOVG_GL3
CFLAGS = -g -I$(S3) -DNANOVG_USE_GLEW -DNANOVG_GL3
# CFLAGS = -g -I$(S3) -DNANOVG_GL3
# CFLAGS = -Wall -O2
LDFLAGS = -lm
LDFLAGS_GUI_LINUX = -lglfw -lX11 -lXi -lXcursor -lEGL -lGL -lGLU -lm -lGLEW

# OBJS = $(B)/main.o $(B)/stbimg.o $(B)/tiff.o $(B)/algorithm.o $(B)/get_opt.o
OBJS = $(B)/main.o $(B)/stbimg.o $(B)/algorithm.o $(B)/get_opt.o
NVOBJS = $(B)/nanovg.o $(B)/nanovg_gl.o $(B)/nanovg_gl_utils.o
TEST_SRCS = mainui.c nanovg.c nanovg_gl.c nanovg_gl_utils.c
TEST_OBJS = $(B)/mainui.o $(NVOBJS)

.PHONY: all

all: build_dir $(B)/sis $(B)/sisui

# $(B)/sis: build_dir $(OBJS)
# 	$(CC) -o $(B)/sis $(OBJS) -l$(LIB_TIFF) $(LDFLAGS)
$(B)/sis: build_dir $(OBJS)
	$(CC) -o $(B)/sis $(OBJS) $(LDFLAGS)
$(B)/get_opt.o: $(S)/get_opt.c $(S)/sis.h
	$(CC) -c -o $(B)/get_opt.o $(CFLAGS) $(S)/get_opt.c
$(B)/algorithm.o: $(S)/algorithm.c $(S)/sis.h
	$(CC) -c -o $(B)/algorithm.o $(CFLAGS) $(S)/algorithm.c
$(B)/stbimg.o: $(S)/stbimg.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/stbimg.o $(CFLAGS) $(S)/stbimg.c
# $(B)/tiff.o: $(S)/tiff.c $(S)/tiff.h $(S)/sis.h
# 	$(CC) -c -o $(B)/tiff.o $(CFLAGS) $(S)/tiff.c
# $(B)/main.o: $(S)/main.c $(S)/stbimg.h $(S)/tiff.h $(S)/sis.h
$(B)/main.o: $(S)/main.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/main.o $(CFLAGS) $(S)/main.c
$(B)/nanovg.o: $(S3)/nanovg.c $(S3)/nanovg.h $(S3)/nanovg_gl.h $(S3)/nanovg_gl_utils.h
	$(CC) -c -o $(B)/nanovg.o $(CFLAGS) $(S3)/nanovg.c
$(B)/nanovg_gl.o: $(S3)/nanovg_gl.h $(S3)/nanovg_gl.c $(S3)/nanovg_gl_utils.h
	$(CC) -c -o $(B)/nanovg_gl.o $(CFLAGS) $(S3)/nanovg_gl.c
$(B)/nanovg_gl_utils.o: $(S3)/nanovg_gl.h $(S3)/nanovg_gl_utils.h $(S3)/nanovg_gl_utils.c
	$(CC) -c -o $(B)/nanovg_gl_utils.o $(CFLAGS) $(S3)/nanovg_gl_utils.c
$(B)/mainui.o: $(S)/mainui.c
	$(CC) -c -o $(B)/mainui.o $(CFLAGS) $(S)/mainui.c
$(B)/sisui: $(B)/mainui.o $(NVOBJS)
	$(CC) -o $@ $^ $(LDFLAGS_GUI_LINUX)

clean:
	rm -rf $(B)
build_dir:
	@mkdir -p $(B)
install:
	install -s $(B)/sis $(BIN_DIR)
	install -m 0644 doc/sis.1 $(MAN_DIR)
