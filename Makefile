#  Copyright 1995, 2021, 2025, 2026 Jörg Bakker
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
  BUILD_TARGET = $(shell uname)
  ## Load OS specific config
  include Make.$(BUILD_TARGET)
  # $(info PREFIX is set to: $(PREFIX))
  SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
  SDL2_LIBS   = $(shell pkg-config --libs sdl2)
## Grab only the directory part for setting RPATH in the executable
  SDL2_LIBDIR = $(shell pkg-config --libs sdl2 | grep -o '\-L[/[:alnum:]]*' | cut -b3-)
else
  include Make.Mingw
endif

## Configure build options
ifeq ($(DEBUG),1)
  # $(warning PREFIX removed in debug mode)
  PREFIX      =
  CFLAGS_LOC += -g
else
  CFLAGS_LOC += -O2 -DNDEBUG
  IFLAGS      = -s
endif

ifdef PREFIX
  CFLAGS_LOC    += -DPREFIX=$(PREFIX)
endif

CFLAGS_GUI    += $(SDL2_CFLAGS)
LDFLAGS_GUI   += $(SDL2_RPATH) $(SDL2_LIBS)

## Build
BIN_DIR       = $(DESTDIR)$(PREFIX)/bin
MAN_DIR       = $(DESTDIR)$(PREFIX)/man/man1
SHARE_DIR     = $(DESTDIR)$(PREFIX)/share/sis
ASSETS_DIR    = $(SHARE_DIR)/assets
DEPTHMAPS_DIR = $(SHARE_DIR)/depthmaps
TEXTURES_DIR  = $(SHARE_DIR)/textures

SRCS     = sis.c stbimg.c algorithm.c get_opt.c $(S3)/liblocate.c $(S3)/cwalk.c
OBJS     = $(B)/sis.o $(B)/stbimg.o $(B)/algorithm.o $(B)/get_opt.o $(B)/liblocate.o $(B)/cwalk.o
OBJS_GUI = $(B)/nanovg_xc.o $(B)/nfd.o

.PHONY: all clean build_dir install uninstall help

all: build_dir $(B)/sis $(B)/sisui

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
$(B)/sis: $(B)/main.o $(OBJS)
	$(CC) -o $(B)/sis $^ $(LDFLAGS)
$(B)/liblocate.o:
	$(CC) -c -o $@ $(CFLAGS_LOC) $(S3)/liblocate.c
$(B)/cwalk.o:
	$(CC) -c -o $@ $(CFLAGS_LOC) $(S3)/cwalk.c
$(B)/nfd.o:
	$(CXX) -c -o $@ $(CFLAGS_GUI) $(S3)/$(NFD_BACKEND)
$(B)/nanovg_xc.o:
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/nanovg_xc.c
$(B)/mainui.o: $(S)/mainui.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/mainui.o $(CFLAGS_GUI) $(S)/mainui.c
$(B)/sisui: $(B)/mainui.o $(OBJS) $(OBJS_GUI)
	$(CXX) -o $@ $^ $(LDFLAGS_GUI)

$(B)/sis.exe:
	$(MINGW_CXX) -o $@ $(MINGW_CFLAGS) $(CFLAGS_LOC) main.c $(SRCS) $(MINGW_LDLIBS)
	cp sys.x86_64-w64-mingw32/lib/*.dll build
$(B)/sisui.exe:
	$(MINGW_CXX) -o $@ $(MINGW_CFLAGS) $(CFLAGS_LOC) $(CFLAGS_GUI) mainui.c $(SRCS) 3rd-party/nanovg_xc.c 3rd-party/nfd_win.cpp $(MINGW_LDLIBS) $(LDFLAGS_GUI)
	cp sys.x86_64-w64-mingw32/lib/*.dll build
	cp sys.x86_64-w64-mingw32/bin/*.dll build

## Packages for deployment
$(B)/sis.zip: all
	zip -r $@ build/sis build/sisui assets/ depthmaps/ textures/
$(B)/sis-setup.exe: install.nsi
	wine "$(HOME)/.wine/drive_c/Program Files (x86)/NSIS/makensis.exe" install.nsi
$(B)/sis.pkg:
	cd macos/siscmd && xcodebuild install
	pkgbuild --root /tmp/siscmd.dst --identifier siscmd $(B)/siscmdComponent.pkg
	cd macos/sisui && xcodebuild install
	pkgbuild --analyze --root /tmp/sisui.dst $(B)/sisuiComponent.plist
	pkgbuild --root /tmp/sisui.dst --component-plist $(B)/sisuiComponent.plist $(B)/sisuiComponent.pkg
	productbuild --synthesize --product macos/requirements.plist --package $(B)/sisuiComponent.pkg --package $(B)/siscmdComponent.pkg $(B)/distribution.plist
	productbuild --distribution $(B)/distribution.plist --resources $(B) --package-path $(B) $@

## Administrative tasks
clean:
	rm -rf $(B)
build_dir:
	@mkdir -p $(B)
install: all
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
	@echo 'CROSS=1:          enable cross compiling, currently mingw only'
