#  Copyright 1995, 2021, 2025 Jörg Bakker
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

## You may uncomment this for more convenient development ...
# DEBUG    = 1
## Build with tcc (preferably set on as a command line parameter of make):
# CC=tcc CFLAGS=-DSTBI_NO_SIMD

S        = .
S3       = $(S)/3rd-party
B        = build

CFLAGS_LOC  = -I$(S3)
LDFLAGS     = -lm

## CROSS can be overriden for cross compiling by a command line option
CROSS    = ""
ifeq ($(CROSS),"")
#  BUILD_TARGET != uname
  BUILD_TARGET = $(shell uname)
else
# $(error Cross compiling not supported, yet)
endif

## Load OS specific config
include Make.$(BUILD_TARGET)
# $(info PREFIX is set to: $(PREFIX))

## Configure external dependencies
## If not cross compiling, non-intern dependencies (glfw) can be set explicitely
## with GLFW_PREFIX on each platform or if this is set to "", it is search by
## pkg-config and also the RPATH is set to this location
ifneq ($(GLFW_PREFIX),"")
  GLFW_CFLAGS = -I$(GLFW_PREFIX)/include
  GLFW_LIBS   = -L$(GLFW_PREFIX)/lib -lglfw
  GLFW_LIBDIR = $(GLFW_PREFIX)/lib
else
  GLFW_CFLAGS = $(shell pkg-config --cflags glfw3)
  GLFW_LIBS   = $(shell pkg-config --libs glfw3)
#  GLFW_CFLAGS != pkg-config --cflags glfw3
#  GLFW_LIBS   != pkg-config --libs glfw3
## Grab only the directory part for setting RPATH in the executable
  GLFW_LIBDIR = $(shell pkg-config --libs glfw3 | grep -o '\-L[/[:alnum:]]*' | cut -b3-)
# GLFW_LIBDIR != pkg-config --libs glfw3 | grep -o '\-L[/[:alnum:]]*' | cut -b3-
endif
ifneq ($(GLFW_LIBDIR),"")
  GLFW_RPATH = -Wl,-rpath,$(GLFW_LIBDIR)
endif

SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL2_LIBS   = $(shell pkg-config --libs sdl2)
SDL2_LIBDIR = $(shell pkg-config --libs sdl2 | grep -o '\-L[/[:alnum:]]*' | cut -b3-)
SDL2_RPATH  = -Wl,-rpath,$(GLFW_LIBDIR)

## Configure build options
ifeq ($(DEBUG),1)
  # $(warning PREFIX removed in debug mode)
  PREFIX      = ""
  CFLAGS_LOC += -g
else
  CFLAGS_LOC += -O2 -DNDEBUG
  IFLAGS      = -s
endif

ifneq ($(PREFIX),"")
  CFLAGS_LOC    += -DPREFIX=$(PREFIX)
endif

ifeq ($(UI_BACKEND),glfw)
  CFLAGS_GUI    += -DGLFW_BACKEND -DNANOVG_GL2 $(GLFW_CFLAGS)
  LDFLAGS_GUI   += $(GLFW_RPATH) $(GLFW_LIBS)
  SOK_BACKOBJ    = ""
else ifeq ($(UI_BACKEND),sdl2)
  CFLAGS_GUI    += -DSDL2_BACKEND $(SDL2_CFLAGS)
  LDFLAGS_GUI   += $(SDL2_RPATH) $(SDL2_LIBS)
  SOK_BACKOBJ    = ""
else
  CFLAGS_GUI    += -DNANOVG_GL3
  SOK_BACKOBJ    = sokol_app.o
endif

# ifeq ($(GL),3)
  # CFLAGS_GUI    += -DNANOVG_GL3
# else ifeq ($(GL),2)
  # CFLAGS_GUI    += -DNANOVG_GL2
# endif

## Build
BIN_DIR       = $(DESTDIR)$(PREFIX)/bin
MAN_DIR       = $(DESTDIR)$(PREFIX)/man/man1
SHARE_DIR     = $(DESTDIR)$(PREFIX)/share/sis
ASSETS_DIR    = $(SHARE_DIR)/assets
DEPTHMAPS_DIR = $(SHARE_DIR)/depthmaps
TEXTURES_DIR  = $(SHARE_DIR)/textures

OBJS     = $(B)/sis.o $(B)/stbimg.o $(B)/algorithm.o $(B)/get_opt.o
OBJS_GUI = $(B)/nanovg_xc.o $(B)/nfdcommon.o $(B)/nfd.o
ifneq ($(SOK_BACKOBJ),"")
  OBJS_GUI += $(B)/$(SOK_BACKOBJ)
endif

.PHONY: all clean build_dir install uninstall help

all: build_dir $(B)/sis $(B)/sisui

$(B)/sis: $(B)/main.o $(OBJS)
	$(CC) -o $(B)/sis $^ $(LDFLAGS)
$(B)/main.o: $(S)/main.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/main.o $(CFLAGS_LOC) $(CFLAGS) $(S)/main.c
$(B)/sis.o: $(S)/sis.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/sis.o $(CFLAGS_LOC) $(CFLAGS) $(S)/sis.c
$(B)/algorithm.o: $(S)/algorithm.c $(S)/sis.h
	$(CC) -c -o $(B)/algorithm.o $(CFLAGS_LOC) $(CFLAGS) $(S)/algorithm.c
$(B)/get_opt.o: $(S)/get_opt.c $(S)/sis.h
	$(CC) -c -o $(B)/get_opt.o $(CFLAGS_LOC) $(CFLAGS) $(S)/get_opt.c
$(B)/stbimg.o: $(S)/stbimg.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/stbimg.o $(CFLAGS_LOC) $(CFLAGS) $(S)/stbimg.c

$(B)/sisui: $(B)/mainui.o $(OBJS) $(OBJS_GUI)
	$(CC) -o $@ $^ $(LDFLAGS_GUI)
$(B)/mainui.o: $(S)/mainui.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/mainui.o $(CFLAGS_GUI) $(S)/mainui.c
$(B)/nanovg_xc.o: $(S3)/nanovg_xc.c $(S3)/nanovg_xc.h
	$(CC) -c -o $(B)/nanovg_xc.o $(CFLAGS_GUI) $(S3)/nanovg_xc.c
# $(B)/nanovg_vtex.o: $(S3)/nanovg_vtex.h $(S3)/nanovg_vtex.c
	# $(CC) -c -o $(B)/nanovg_vtex.o $(CFLAGS_GUI) $(S3)/nanovg_vtex.c
# $(B)/nanovg_gl_utils.o: $(S3)/nanovg_gl.h $(S3)/nanovg_gl_utils.h $(S3)/nanovg_gl_utils.c
	# $(CC) -c -o $(B)/nanovg_gl_utils.o $(CFLAGS_GUI) $(S3)/nanovg_gl_utils.c
$(B)/nfdcommon.o:
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/nfd_common.c
$(B)/nfd.o:
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/$(NFD_BACKEND)
$(B)/$(SOK_BACKOBJ):
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/$(SOK_BACKEND)

$(B)/nfd_test:
	$(CC) -o build/nfd_test -I 3rd-party/ nfd_test.c build/nfdcommon.o build/nfd.o $(LDFLAGS_GUI)

## Administrative tasks
clean:
	rm -rf $(B)
build_dir:
	@mkdir -p $(B)
install:
	mkdir -p $(BIN_DIR) $(MAN_DIR) $(ASSETS_DIR) $(DEPTHMAPS_DIR) $(TEXTURES_DIR)
	install $(IFLAGS) $(B)/sis $(B)/sisui $(BIN_DIR)
	install -m 0644 assets/* $(ASSETS_DIR)
	install -m 0644 depthmaps/* $(DEPTHMAPS_DIR)
	install -m 0644 textures/* $(TEXTURES_DIR)
	install -m 0644 doc/sis.1 $(MAN_DIR)
uninstall:
	rm -rf $(BIN_DIR)/sis $(BIN_DIR)/sisui $(MAN_DIR)/sis.1 $(SHARE_DIR)
help:
	@echo Available config options
	@echo 'PREFIX=<prefix>:  set prefix for installation (set on build and install targets)'
	@echo 'DEBUG=1:          debug build, sisui can be run from local directory without install'
	@echo 'UI_BACKEND=glfw|sdl2:  use GLFW for creating graphics context instead of sokol'
	@echo 'CROSS=<platform>: set target platform for cross compiling'
