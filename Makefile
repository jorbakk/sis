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

S        = .
S3       = $(S)/3rd-party
B        = build

CFLAGS   = -I$(S3)
LDFLAGS  = -lm

include Make.$(shell uname)

ifeq ($(DEBUG), 1)
  $(warning PREFIX removed in debug mode)
  PREFIX  = ""
  CFLAGS += -g
else
  CFLAGS += -O2 -DNDEBUG
  IFLAGS  = -s
endif

ifneq ($(PREFIX), "")
  CFLAGS    += -DPREFIX=$(PREFIX)
endif

ifeq ($(UI_BACKEND), glfw)
  CFLAGS_GUI    += -DGLFW_BACKEND -DNANOVG_GL2
  LDFLAGS_GUI   += -lglfw
else
  CFLAGS_GUI    += -DNANOVG_GL3
endif

# ifeq ($(GL), 3)
  # CFLAGS_GUI    += -DNANOVG_GL3
# else ifeq ($(GL), 2)
  # CFLAGS_GUI    += -DNANOVG_GL2
# endif

BIN_DIR       = $(DESTDIR)$(PREFIX)/bin
MAN_DIR       = $(DESTDIR)$(PREFIX)/man/man1
SHARE_DIR     = $(DESTDIR)$(PREFIX)/share/sis
ASSETS_DIR    = $(SHARE_DIR)/assets
DEPTHMAPS_DIR = $(SHARE_DIR)/depthmaps
TEXTURES_DIR  = $(SHARE_DIR)/textures

OBJS     = $(B)/sis.o $(B)/stbimg.o $(B)/algorithm.o $(B)/get_opt.o
OBJS_GUI = $(B)/nanovg.o $(B)/nanovg_gl.o $(B)/nanovg_gl_utils.o $(B)/sokol_app.o $(B)/nfdcommon.o $(B)/nfd.o

.PHONY: all clean build_dir install uninstall

all: build_dir $(B)/sis $(B)/sisui

$(B)/sis: $(B)/main.o $(OBJS)
	$(CC) -o $(B)/sis $^ $(LDFLAGS)
$(B)/main.o: $(S)/main.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/main.o $(CFLAGS) $(S)/main.c
$(B)/sis.o: $(S)/sis.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/sis.o $(CFLAGS) $(S)/sis.c
$(B)/algorithm.o: $(S)/algorithm.c $(S)/sis.h
	$(CC) -c -o $(B)/algorithm.o $(CFLAGS) $(S)/algorithm.c
$(B)/get_opt.o: $(S)/get_opt.c $(S)/sis.h
	$(CC) -c -o $(B)/get_opt.o $(CFLAGS) $(S)/get_opt.c
$(B)/stbimg.o: $(S)/stbimg.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/stbimg.o $(CFLAGS) $(S)/stbimg.c

$(B)/sisui: $(B)/mainui.o $(OBJS) $(OBJS_GUI)
	$(CC) -o $@ $^ $(LDFLAGS_GUI)
$(B)/mainui.o: $(S)/mainui.c $(S)/stbimg.h $(S)/sis.h
	$(CC) -c -o $(B)/mainui.o $(CFLAGS_GUI) $(S)/mainui.c
$(B)/nanovg.o: $(S3)/nanovg.c $(S3)/nanovg.h $(S3)/nanovg_gl.h $(S3)/nanovg_gl_utils.h
	$(CC) -c -o $(B)/nanovg.o $(CFLAGS_GUI) $(S3)/nanovg.c
$(B)/nanovg_gl.o: $(S3)/nanovg_gl.h $(S3)/nanovg_gl.c $(S3)/nanovg_gl_utils.h
	$(CC) -c -o $(B)/nanovg_gl.o $(CFLAGS_GUI) $(S3)/nanovg_gl.c
$(B)/nanovg_gl_utils.o: $(S3)/nanovg_gl.h $(S3)/nanovg_gl_utils.h $(S3)/nanovg_gl_utils.c
	$(CC) -c -o $(B)/nanovg_gl_utils.o $(CFLAGS_GUI) $(S3)/nanovg_gl_utils.c
$(B)/sokol_app.o:
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/$(APP_BACKEND)
$(B)/nfdcommon.o:
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/nfd_common.c
$(B)/nfd.o:
	$(CC) -c -o $@ $(CFLAGS_GUI) $(S3)/$(NFD_BACKEND)

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
